///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2019 DNEG
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG nor the names
// of its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

#include "Compiler.h"

#include "PointExecutable.h"
#include "VolumeExecutable.h"

#include <openvdb_ax/ast/Scanners.h>
#include <openvdb_ax/codegen/Functions.h>
#include <openvdb_ax/codegen/PointComputeGenerator.h>
#include <openvdb_ax/codegen/VolumeComputeGenerator.h>
#include <openvdb_ax/Exceptions.h>

#include <openvdb/Exceptions.h>

#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/InitializePasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ManagedStatic.h> // llvm_shutdown
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/SourceMgr.h> // SMDiagnostic
#include <llvm/Support/TargetSelect.h>
#include <llvm/Config/llvm-config.h>

// @note  As of adding support for LLVM 5.0 we not longer explicitly
// perform standrd compiler passes (-std-compile-opts) based on the changes
// to the opt binary in the llvm codebase (tools/opt.cpp). We also no
// longer explicitly perform:
//  - llvm::createStripSymbolsPass()
// And have never performed any specific target machine analysis passes
//
// @todo  Properly identify the IPO passes that we would benefit from using
// as well as what user controls would otherwise be appropriate

#include <llvm/Transforms/IPO.h> // Inter-procedural optimization passes
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Mangler.h>

#include <tbb/mutex.h>

#include <unordered_map>


namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {


namespace {
// Declare this at file scope to ensure thread-safe initialization.
tbb::mutex sInitMutex;
bool sIsInitialized = false;
bool sShutdown = false;
}


bool isInitialized()
{
    tbb::mutex::scoped_lock lock(sInitMutex);
    return sIsInitialized;
}

void initialize()
{
    tbb::mutex::scoped_lock lock(sInitMutex);
    if (sIsInitialized) return;

    if (sShutdown) {
        OPENVDB_THROW(LLVMInitialisationError, "Unable to re-initialize after uninitialize has been called.");
    }

    // Init JIT

    if (llvm::InitializeNativeTarget() ||
        llvm::InitializeNativeTargetAsmPrinter() ||
        llvm::InitializeNativeTargetAsmParser()) {
        OPENVDB_THROW(LLVMInitialisationError, "Failed to initialize for JIT");
    }

    LLVMLinkInMCJIT();

    // Initialize passes
    llvm::PassRegistry& registry = *llvm::PassRegistry::getPassRegistry();
    llvm::initializeCore(registry);
    llvm::initializeScalarOpts(registry);
    llvm::initializeObjCARCOpts(registry);
    llvm::initializeVectorization(registry);
    llvm::initializeIPO(registry);
    llvm::initializeAnalysis(registry);
    llvm::initializeTransformUtils(registry);
    llvm::initializeInstCombine(registry);
#if LLVM_VERSION_MAJOR > 6
    llvm::initializeAggressiveInstCombine(registry);
#endif
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
    // For codegen passes, only passes that do IR to IR transformation are
    // supported.
#if LLVM_VERSION_MAJOR > 5
    llvm::initializeExpandMemCmpPassPass(registry);
#endif
    llvm::initializeScalarizeMaskedMemIntrinPass(registry);
    llvm::initializeCodeGenPreparePass(registry);
    llvm::initializeAtomicExpandPass(registry);
    llvm::initializeRewriteSymbolsLegacyPassPass(registry);
    llvm::initializeWinEHPreparePass(registry);
    llvm::initializeDwarfEHPreparePass(registry);
    llvm::initializeSafeStackLegacyPassPass(registry);
    llvm::initializeSjLjEHPreparePass(registry);
    llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registry);
    llvm::initializeGlobalMergePass(registry);
#if LLVM_VERSION_MAJOR > 6
    llvm::initializeIndirectBrExpandPassPass(registry);
#endif
#if LLVM_VERSION_MAJOR > 7
    llvm::initializeInterleavedLoadCombinePass(registry);
#endif
    llvm::initializeInterleavedAccessPass(registry);
#if LLVM_VERSION_MAJOR > 5
    llvm::initializeEntryExitInstrumenterPass(registry);
    llvm::initializePostInlineEntryExitInstrumenterPass(registry);
#else
    llvm::initializeCountingFunctionInserterPass(registry);
#endif
    llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
    llvm::initializeExpandReductionsPass(registry);
#if LLVM_VERSION_MAJOR > 6
    llvm::initializeWasmEHPreparePass(registry);
#endif
#if LLVM_VERSION_MAJOR > 5
    llvm::initializeWriteBitcodePassPass(registry);
#endif

    sIsInitialized = true;
}

void uninitialize()
{
    tbb::mutex::scoped_lock lock(sInitMutex);
    if (!sIsInitialized) return;

    // @todo consider replacing with storage to Support/InitLLVM
    llvm::llvm_shutdown();

    sIsInitialized = false;
    sShutdown = true;
}

namespace
{

void addStandardLinkPasses(llvm::legacy::PassManagerBase& passes)
{
    llvm::PassManagerBuilder builder;
    builder.VerifyInput = true;
    builder.Inliner = llvm::createFunctionInliningPass();
    builder.populateLTOPassManager(passes);
}

/// This routine adds optimization passes based on selected optimization level
///
void addOptimizationPasses(llvm::legacy::PassManagerBase& passes,
                           llvm::legacy::FunctionPassManager& functionPasses,
                           llvm::TargetMachine* targetMachine,
                           const unsigned optLevel,
                           const unsigned sizeLevel,
                           const bool disableInline = false,
                           const bool disableUnitAtATime = false,
                           const bool disableLoopUnrolling = false,
                           const bool disableLoopVectorization = false,
                           const bool disableSLPVectorization = false)
{
    llvm::PassManagerBuilder builder;
    builder.OptLevel = optLevel;
    builder.SizeLevel = sizeLevel;

    if (disableInline) {
        // No inlining pass
    } else if (optLevel > 1) {
        builder.Inliner =
            llvm::createFunctionInliningPass(optLevel, sizeLevel,
                /*DisableInlineHotCallSite*/false);
    } else {
        builder.Inliner = llvm::createAlwaysInlinerLegacyPass();
    }

#if LLVM_VERSION_MAJOR < 9
    // Enable IPO. This corresponds to gcc's -funit-at-a-time
    builder.DisableUnitAtATime = disableUnitAtATime;
#endif

    // Disable loop unrolling in all relevant passes
    builder.DisableUnrollLoops =
        disableLoopUnrolling ? disableLoopUnrolling : optLevel == 0;

    // See the following link for more info on vectorizers
    // http://llvm.org/docs/Vectorizers.html

    // (-vectorize-loops, -loop-vectorize)
    builder.LoopVectorize =
        disableLoopVectorization ? false : optLevel > 1 && sizeLevel < 2;
    builder.SLPVectorize =
        disableSLPVectorization ? false : optLevel > 1 && sizeLevel < 2;

    // If a target machine is provided, allow the target to modify the pass manager
    // e.g. by calling PassManagerBuilder::addExtension.
    if (targetMachine) {
        targetMachine->adjustPassManager(builder);
    }

    builder.populateFunctionPassManager(functionPasses);
    builder.populateModulePassManager(passes);
}


void LLVMoptimise(llvm::Module* module,
                  const unsigned optLevel,
                  const unsigned sizeLevel,
                  const bool verify = false)
{
    // Pass manager setup and IR optimisations - Do target independent optimisations
    // only - i.e. the following do not require an llvm TargetMachine analysis pass

    llvm::legacy::PassManager passes;
    const llvm::Triple moduleTriple(module->getTargetTriple());
    llvm::TargetLibraryInfoImpl tlii(moduleTriple);
    passes.add(new llvm::TargetLibraryInfoWrapperPass(tlii));

    // Add internal analysis passes from the target machine.
    passes.add(llvm::createTargetTransformInfoWrapperPass(llvm::TargetIRAnalysis()));

    llvm::legacy::FunctionPassManager functionPasses(module);
    functionPasses.add(llvm::createTargetTransformInfoWrapperPass(llvm::TargetIRAnalysis()));

    if (verify) functionPasses.add(llvm::createVerifierPass());

    addStandardLinkPasses(passes);
    addOptimizationPasses(passes, functionPasses, nullptr, optLevel, sizeLevel);

    functionPasses.doInitialization();
    for (llvm::Function& function : *module) {
      functionPasses.run(function);
    }
    functionPasses.doFinalization();

    if (verify) passes.add(llvm::createVerifierPass());
    passes.run(*module);
}

void initializeGlobalFunctions(const codegen::FunctionRegistry& registry,
                               llvm::ExecutionEngine& engine,
                               llvm::Module& module)
{
    /// @note  This is a copy of ExecutionEngine::getMangledName. LLVM's ExecutionEngine
    ///   provides two signatures for updating global mappings, one which takes a void* and
    ///   another which takes a uint64_t address. When providing function mappings,
    ///   it is potentially unsafe to cast pointers-to-functions to pointers-to-objects
    ///   as they are not guaranteed to have the same size on some (albiet non "standard")
    ///   platforms. getMangledName is protected, so a copy exists here to allows us to
    ///   call the uint64_t method.
    /// @note  This is only caught by -pendantic so this work around may be overkill
    auto getMangledName = [](const llvm::GlobalValue* GV,
                        const llvm::ExecutionEngine& E) -> std::string
    {
        llvm::SmallString<128> FullName;
        const llvm::DataLayout& DL =
            GV->getParent()->getDataLayout().isDefault()
                ? E.getDataLayout()
                : GV->getParent()->getDataLayout();
        llvm::Mangler::getNameWithPrefix(FullName, GV->getName(), DL);
        return FullName.str();
    };

    /// @note  Could use InstallLazyFunctionCreator here instead as follows:
    ///
    /// engine.InstallLazyFunctionCreator([](const std::string& name) -> void * {
    ///    // Loop through register and find matching symbol
    /// });
    ///
    /// However note that if functions have been compiled with mLazyFunctions that the
    /// below code using addGlobalMapping() only adds mapping for instantiated
    /// functions anyway.
    ///
    /// @note  Depending on how functions are inserted into LLVM (Linkage Type) in
    ///        the future, InstallLazyFunctionCreator may be required
    for (const auto& iter : registry.map()) {
        const codegen::FunctionBase::Ptr function = iter.second.function();
        if (!function) continue;

        const codegen::FunctionBase::FunctionList& list = function->list();
        for (const codegen::FunctionSignatureBase::Ptr& signature : list) {

            // If a function pointer exists, the function has external linkage
            codegen::FunctionSignatureBase::VoidFPtr functionPtr = signature->functionPointer();
            if (!functionPtr) continue;

            // llvmFunction may not exists if compiled without mLazyFunctions
            const llvm::Function* llvmFunction = module.getFunction(signature->symbolName());
            if (!llvmFunction) continue;

            const std::string mangled =
                getMangledName(llvm::cast<llvm::GlobalValue>(llvmFunction), engine);

            // error if updateGlobalMapping returned a previously mapped address, as we've
            // overwritten something

            const uint64_t address = reinterpret_cast<uint64_t>(functionPtr);
            const uint64_t oldAddress = engine.updateGlobalMapping(mangled, address);

            if (oldAddress != 0 && oldAddress != address) {
                OPENVDB_THROW(LLVMFunctionError, "Function registry mapping error - multiple functions "
                    "are using the same symbol \"" + signature->symbolName() + "\".");
            }
        }
    }
}

void optimiseAndVerify(llvm::Module* module, const bool verify, const CompilerOptions::OptLevel optLevel)
{
    if (verify) {
        llvm::raw_os_ostream out(std::cout);
        if (llvm::verifyModule(*module, &out)) {
            OPENVDB_THROW(LLVMIRError, "LLVM IR is not valid.");
        }
    }

    switch (optLevel) {
        case CompilerOptions::OptLevel::O0 : {
            LLVMoptimise(module, 0, 0, verify);
            break;
        }
        case CompilerOptions::OptLevel::O1 : {
            LLVMoptimise(module, 1, 0, verify);
            break;
        }
        case CompilerOptions::OptLevel::O2 : {
            LLVMoptimise(module, 2, 0, verify);
            break;
        }
        case CompilerOptions::OptLevel::Os : {
            LLVMoptimise(module, 2, 1, verify);
            break;
        }
        case CompilerOptions::OptLevel::Oz : {
            LLVMoptimise(module, 2, 2, verify);
            break;
        }
        case CompilerOptions::OptLevel::O3 : {
            LLVMoptimise(module, 3, 0, verify);
            break;
        }
        case CompilerOptions::OptLevel::NONE :
        default             : {}
    }
}

void verifyTypedAccesses(const ast::Tree& tree)
{
    // verify the attributes and external variables requested in the syntax tree
    // only have a single type. Note that the executer will also throw a runtime
    // error if the same attribute is accessed with different types, but as that's
    // currently not a valid state on a PointDataGrid, error in compilation as well
    // @todo - introduce a framework for supporting custom preprocessors

    std::unordered_map<std::string, std::string> nameType;

    auto attributeOp =
        [&nameType](const ast::Attribute& node) -> bool {
            auto iter = nameType.find(node.name());
            if (iter == nameType.end()) {
                nameType[node.name()] = node.typestr();
            }
            else if (iter->second != node.typestr()) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile ambiguous @ parameters. "
                    "\"" + node.name() + "\" has been accessed with different types.");
            }
            return true;
        };

    ast::visitNodeType<ast::Attribute>(tree, attributeOp);

    nameType.clear();

    auto externalOp =
        [&nameType](const ast::ExternalVariable& node) -> bool {
            auto iter = nameType.find(node.name());
            if (iter == nameType.end()) {
                nameType[node.name()] = node.typestr();
            }
            else if (iter->second != node.typestr()) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile ambiguous $ parameters. "
                    "\"" + node.name() + "\" has been accessed with different types.");
            }

            // Error on string lookups as we don't support these
            // @todo ... support these
            if (node.typestr() == openvdb::typeNameAsString<std::string>()) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile string $ parameter "
                    "\"" + node.name() + "\". string lookups are not currently supported.");
            }
            return true;
        };

    ast::visitNodeType<ast::ExternalVariable>(tree, externalOp);
}

inline void
registerAccesses(const codegen::SymbolTable& globals, const AttributeRegistry& registry)
{
    std::string name, type;

    for (const auto& global : globals.map()) {

        // detect if this global variable is an attribute access
        const std::string& token = global.first;
        if (!ast::Attribute::nametypeFromToken(token, &name, &type)) continue;

        const ast::tokens::CoreType typetoken =
            ast::tokens::tokenFromTypeString(type);

        // add the access to the registry - this will force the executables
        // to always request or create the data type

        const size_t index = registry.accessIndex(name, typetoken);

        // should always be a GlobalVariable.
        assert(llvm::isa<llvm::GlobalVariable>(global.second));

        // Assign the attribute index global a valid index.
        // @note executionEngine->addGlobalMapping() can also be used if the indices
        // ever need to vary positions without having to force a recompile (previously
        // was used unnecessarily)

        llvm::GlobalVariable* variable =
            llvm::cast<llvm::GlobalVariable>(global.second);
        assert(variable->getValueType()->isIntegerTy(64));

        variable->setInitializer(llvm::ConstantInt::get(variable->getValueType(), index));
        variable->setConstant(true); // is not written to at runtime
    }
}

template <typename T>
inline llvm::Constant*
initializeMetadataPtr(CustomData& data,
    const std::string& name,
    llvm::LLVMContext& C)
{
    TypedMetadata<T>* meta = data.getOrInsertData<TypedMetadata<T>>(name);
    if (meta) return codegen::LLVMType<uintptr_t>::get(C, meta->value());
    return nullptr;
}

inline void
registerExternalGlobals(const codegen::SymbolTable& globals, CustomData::Ptr& data, llvm::LLVMContext& C)
{
    std::string name, typestr;
    for (const auto& global : globals.map()) {

        const std::string& token = global.first;
        if (!ast::ExternalVariable::nametypeFromToken(token, &name, &typestr)) continue;

        const ast::tokens::CoreType typetoken =
            ast::tokens::tokenFromTypeString(typestr);

        // if we have any external variables, the custom data must be initialized to at least hold
        // zero values (initialized by the default metadata types)
        if (!data) data.reset(new CustomData);

        // should always be a GlobalVariable.
        assert(llvm::isa<llvm::GlobalVariable>(global.second));

        llvm::GlobalVariable* variable = llvm::cast<llvm::GlobalVariable>(global.second);
        assert(variable->getValueType() == codegen::LLVMType<uintptr_t>::get(C));

        auto initializerFromToken =
            [&](const ast::tokens::CoreType type) -> llvm::Constant* {
            switch (type) {
                case ast::tokens::BOOL    : return initializeMetadataPtr<bool>(*data, name, C);
                case ast::tokens::CHAR    : return initializeMetadataPtr<char>(*data, name, C);
                case ast::tokens::SHORT   : return initializeMetadataPtr<int16_t>(*data, name, C);
                case ast::tokens::INT     : return initializeMetadataPtr<int32_t>(*data, name, C);
                case ast::tokens::LONG    : return initializeMetadataPtr<int64_t>(*data, name, C);
                case ast::tokens::FLOAT   : return initializeMetadataPtr<float>(*data, name, C);
                case ast::tokens::DOUBLE  : return initializeMetadataPtr<double>(*data, name, C);
                case ast::tokens::VEC2I   : return initializeMetadataPtr<math::Vec2<int32_t>>(*data, name, C);
                case ast::tokens::VEC2F   : return initializeMetadataPtr<math::Vec2<float>>(*data, name, C);
                case ast::tokens::VEC2D   : return initializeMetadataPtr<math::Vec2<double>>(*data, name, C);
                case ast::tokens::VEC3I   : return initializeMetadataPtr<math::Vec3<int32_t>>(*data, name, C);
                case ast::tokens::VEC3F   : return initializeMetadataPtr<math::Vec3<float>>(*data, name, C);
                case ast::tokens::VEC3D   : return initializeMetadataPtr<math::Vec3<double>>(*data, name, C);
                case ast::tokens::VEC4I   : return initializeMetadataPtr<math::Vec4<int32_t>>(*data, name, C);
                case ast::tokens::VEC4F   : return initializeMetadataPtr<math::Vec4<float>>(*data, name, C);
                case ast::tokens::VEC4D   : return initializeMetadataPtr<math::Vec4<double>>(*data, name, C);
                case ast::tokens::MAT3F   : return initializeMetadataPtr<math::Mat3<float>>(*data, name, C);
                case ast::tokens::MAT3D   : return initializeMetadataPtr<math::Mat3<double>>(*data, name, C);
                case ast::tokens::MAT4F   : return initializeMetadataPtr<math::Mat4<float>>(*data, name, C);
                case ast::tokens::MAT4D   : return initializeMetadataPtr<math::Mat4<double>>(*data, name, C);
                //case ast::tokens::STRING  : @todo
                case ast::tokens::UNKNOWN :
                default      : {
                    // grammar guarantees this is unreachable as long as all types are supported
                    OPENVDB_THROW(LLVMTypeError, "Attribute Type unsupported or not recognised");
                }
            }
        };

        llvm::Constant* initializer = initializerFromToken(typetoken);
        if (!initializer) {
            OPENVDB_THROW(AXCompilerError, "Custom data \"" + name + "\" already exists with a "
                "different type.");
        }

        variable->setInitializer(initializer);
        variable->setConstant(true); // is not written to at runtime
    }
}

struct PointDefaultModifier :
    public openvdb::ax::ast::Visitor<PointDefaultModifier, /*non-const*/false>
{
    using openvdb::ax::ast::Visitor<PointDefaultModifier, false>::traverse;
    using openvdb::ax::ast::Visitor<PointDefaultModifier, false>::visit;

    PointDefaultModifier() = default;
    virtual ~PointDefaultModifier() = default;

    const std::set<std::string> autoVecAttribs {"P", "v", "N", "Cd"};

    bool visit(ast::Attribute* attrib) {
        if (!attrib->inferred()) return true;
        if (autoVecAttribs.find(attrib->name()) == autoVecAttribs.end()) return true;

        openvdb::ax::ast::Attribute::UniquePtr
            replacement(new openvdb::ax::ast::Attribute(attrib->name(), ast::tokens::VEC3F, true));
        if (!attrib->replace(replacement.get())) {
            OPENVDB_THROW(AXExecutionError,
                "Auto conversion of inferred attributes failed.");
        }
        replacement.release();

        return true;
    }
};

} // anonymous namespace

/////////////////////////////////////////////////////////////////////////////

Compiler::Compiler(const CompilerOptions& options,
                   const std::function<ast::Tree::Ptr(const char*)>& parser)
    : mContext()
    , mCompilerOptions(options)
    , mParser(parser)
    , mFunctionRegistry()
{
    mContext.reset(new llvm::LLVMContext);
    mFunctionRegistry = codegen::createDefaultRegistry(&options.mFunctionOptions);
}

Compiler::UniquePtr Compiler::create(const CompilerOptions &options,
                                     const std::function<ast::Tree::Ptr (const char *)> &parser)
{
    UniquePtr compiler(new Compiler(options, parser));
    return compiler;
}

void Compiler::setFunctionRegistry(std::unique_ptr<codegen::FunctionRegistry>&& functionRegistry)
{
    mFunctionRegistry = std::move(functionRegistry);
}


template<>
PointExecutable::Ptr
Compiler::compile<PointExecutable>(const ast::Tree& syntaxTree,
                                   const CustomData::Ptr customData,
                                   std::vector<std::string>* warnings)
{
    openvdb::SharedPtr<ast::Tree> tree(syntaxTree.copy());
    PointDefaultModifier modifier;
    modifier.traverse(tree.get());

    verifyTypedAccesses(*tree);

    // initialize the module and generate LLVM IR

    std::unique_ptr<llvm::Module> module(new llvm::Module("module", *mContext));

    codegen::PointComputeGenerator
        codeGenerator(*module, mCompilerOptions.mFunctionOptions,
            *mFunctionRegistry, warnings);

    AttributeRegistry::Ptr registry = codeGenerator.generate(*tree);

    // map accesses (always do this prior to optimising as globals may be removed)

    registerAccesses(codeGenerator.globals(), *registry);

    CustomData::Ptr validCustomData(customData);
    registerExternalGlobals(codeGenerator.globals(), validCustomData, *mContext);

    // optimise

    llvm::Module* modulePtr = module.get();
    optimiseAndVerify(modulePtr, mCompilerOptions.mVerify, mCompilerOptions.mOptLevel);

    // create the llvm execution engine which will build our function pointers

    std::string error;
    std::shared_ptr<llvm::ExecutionEngine>
        executionEngine(llvm::EngineBuilder(std::move(module))
            .setEngineKind(llvm::EngineKind::JIT)
            .setErrorStr(&error)
            .create());

    if (!executionEngine) {
        OPENVDB_THROW(AXExecutionError, "Failed to create ExecutionEngine: " + error);
    }

    // map functions

    initializeGlobalFunctions(*mFunctionRegistry, *executionEngine, *modulePtr);

    // finalize mapping

    executionEngine->finalizeObject();

    // get the built function pointers

    const std::vector<std::string> functionNames {
        codegen::PointKernel::getDefaultName(),
        codegen::PointRangeKernel::getDefaultName()
    };

    std::unordered_map<std::string, uint64_t> functionMap;

    for (const std::string& name : functionNames) {
        const uint64_t address = executionEngine->getFunctionAddress(name);
        if (!address) {
            OPENVDB_THROW(AXCompilerError, "Failed to compile compute function \"" + name + "\"");
        }
        functionMap[name] = address;
    }

    // create final executable object
    PointExecutable::Ptr executable(new PointExecutable(executionEngine, mContext, registry, validCustomData,
        functionMap));
    return executable;
}

template<>
VolumeExecutable::Ptr
Compiler::compile<VolumeExecutable>(const ast::Tree& syntaxTree,
                                    const CustomData::Ptr customData,
                                    std::vector<std::string>* warnings)
{
    verifyTypedAccesses(syntaxTree);

    // initialize the module and generate LLVM IR

    std::unique_ptr<llvm::Module> module(new llvm::Module("module", *mContext));

    codegen::VolumeComputeGenerator
        codeGenerator(*module, mCompilerOptions.mFunctionOptions,
            *mFunctionRegistry, warnings);

    AttributeRegistry::Ptr registry = codeGenerator.generate(syntaxTree);

    // map accesses (always do this prior to optimising as globals may be removed)

    registerAccesses(codeGenerator.globals(), *registry);

    CustomData::Ptr validCustomData(customData);
    registerExternalGlobals(codeGenerator.globals(), validCustomData, *mContext);

    llvm::Module* modulePtr = module.get();
    optimiseAndVerify(modulePtr, mCompilerOptions.mVerify, mCompilerOptions.mOptLevel);

    std::string error;
    std::shared_ptr<llvm::ExecutionEngine>
        executionEngine(llvm::EngineBuilder(std::move(module))
            .setEngineKind(llvm::EngineKind::JIT)
            .setErrorStr(&error)
            .create());

    if (!executionEngine) {
        OPENVDB_THROW(AXExecutionError, "Failed to create ExecutionEngine: " + error);
    }

    // map functions

    initializeGlobalFunctions(*mFunctionRegistry, *executionEngine,
        *modulePtr);

    // finalize mapping

    executionEngine->finalizeObject();

    const std::string name = codegen::VolumeKernel::getDefaultName();
    const uint64_t address = executionEngine->getFunctionAddress(name);
    if (!address) {
        OPENVDB_THROW(AXCompilerError, "Failed to compile compute function \"" + name + "\"");
    }

    std::unordered_map<std::string, uint64_t> functionMap;
    functionMap[name] = address;

    // create final executable object
    VolumeExecutable::Ptr
        executable(new VolumeExecutable(executionEngine, mContext, registry, validCustomData,
            functionMap));
    return executable;
}


}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
