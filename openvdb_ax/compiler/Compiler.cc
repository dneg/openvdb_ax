///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2018 DNEG Visual Effects
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG Visual Effects nor the names
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
#include <openvdb_ax/codegen/FunctionRegistry.h>
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

#include <tbb/mutex.h>


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
    llvm::initializeInstrumentation(registry);
    llvm::initializeTarget(registry);
    // For codegen passes, only passes that do IR to IR transformation are
    // supported.
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
    llvm::initializeInterleavedAccessPass(registry);
    llvm::initializeCountingFunctionInserterPass(registry);
    llvm::initializeUnreachableBlockElimLegacyPassPass(registry);
    llvm::initializeExpandReductionsPass(registry);

    sIsInitialized = true;
}

void uninitialize()
{
    tbb::mutex::scoped_lock lock(sInitMutex);
    if (!sIsInitialized) return;

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

    // Enable IPO. This corresponds to gcc's -funit-at-a-time
    builder.DisableUnitAtATime = disableUnitAtATime;

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
            void* functionPtr = signature->functionPointer();
            if (!functionPtr) continue;

            // llvmFunction may not exists if compiled without mLazyFunctions
            const llvm::Function* llvmFunction = module.getFunction(signature->symbolName());
            if (!llvmFunction) continue;

            // error if updateGlobalMapping returned a previously mapped address, as we've
            // overwritten something

            const uint64_t oldAddress = engine.updateGlobalMapping(llvmFunction, functionPtr);
            if (oldAddress != 0 && reinterpret_cast<void*>(oldAddress) != functionPtr) {
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
        [&nameType](const ast::Attribute& node) {
            auto iter = nameType.find(node.mName);
            if (iter == nameType.end()) {
                nameType[node.mName] = node.mType;
            }
            else if (iter->second != node.mType) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile ambiguous @ parameters. "
                    "\"" + node.mName + "\" has been accessed with different types.");
            }
        };

    ast::visitNodeType<ast::Attribute>(tree, attributeOp);

    nameType.clear();

    auto externalOp =
        [&nameType](const ast::ExternalVariable& node) {
            auto iter = nameType.find(node.mName);
            if (iter == nameType.end()) {
                nameType[node.mName] = node.mType;
            }
            else if (iter->second != node.mType) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile ambiguous $ parameters. "
                    "\"" + node.mName + "\" has been accessed with different types.");
            }

            // Error on string lookups as we don't support these
            // @todo ... support these
            if (node.mType == openvdb::typeNameAsString<std::string>()) {
                OPENVDB_THROW(AXCompilerError, "Failed to compile string $ parameter "
                    "\"" + node.mName + "\". string lookups are not currently supported.");
            }
        };

    ast::visitNodeType<ast::ExternalVariable>(tree, externalOp);
}

template <typename RegistryT>
inline typename RegistryT::Ptr
registerAccesses(const codegen::SymbolTable& globals, const ast::Tree& tree)
{
    // get the attributes targets

    std::set<std::string> targets;

    auto op =
        [&targets](const ast::AssignExpression& node) {
            const auto attribute =
                std::dynamic_pointer_cast<ast::Attribute>(node.mVariable);
            if (!attribute) return;
            targets.insert(attribute->mName);
        };

    ast::visitNodeType<ast::AssignExpression>(tree, op);

    typename RegistryT::Ptr registry(new RegistryT);

    std::string name, type;

    for (const auto& global : globals.map()) {

        // detect if this global variable is an attribute access

        const std::string& token = global.first;
        if (!codegen::isGlobalAttributeAccess(token, name, type)) continue;

        // select whether we are writing to this attribute.

        bool write = targets.count(name);

        // add the access to the registry - this will force the executables
        // to always request or create the data type

        const size_t index = registry->addData(name, type, write);

        // should always be a GlobalVariable.
        assert(llvm::isa<llvm::GlobalVariable>(global.second));

        // Assign the attribute index global a valid index.
        // @note executionEngine->addGlobalMapping() can also be used if the indices
        // every need to vary positions without having to force a recompile (previously
        // was used unnecessarily)

        llvm::GlobalVariable* variable = llvm::cast<llvm::GlobalVariable>(global.second);
        assert(variable->getValueType()->isIntegerTy(64));

        variable->setInitializer(llvm::ConstantInt::get(variable->getValueType(), index));
        variable->setConstant(true); // is not written to at runtime
    }

    return registry;
}

inline void
registerExternalGlobals(const codegen::SymbolTable& globals, CustomData::Ptr& data, llvm::LLVMContext& C)
{
    std::string name, type;
    for (const auto& global : globals.map()) {

        const std::string& token = global.first;
        if (!codegen::isGlobalExternalAccess(token, name, type)) continue;

        // if we have any external variables, the custom data must be initialized to at least hold
        // zero values (initialized by the default metadata types)
        if (!data) data.reset(new CustomData);

        // should always be a GlobalVariable.
        assert(llvm::isa<llvm::GlobalVariable>(global.second));

        llvm::GlobalVariable* variable = llvm::cast<llvm::GlobalVariable>(global.second);
        assert(variable->getValueType() == codegen::LLVMType<uintptr_t>::get(C));
        llvm::Constant* initializer = nullptr;

        if (type == typeNameAsString<bool>()) {
            TypedMetadata<bool>* meta =
                data->getOrInsertData<TypedMetadata<bool>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<int16_t>()) {
            TypedMetadata<int16_t>* meta =
                data->getOrInsertData<TypedMetadata<int16_t>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<int32_t>()) {
            TypedMetadata<int32_t>* meta =
                data->getOrInsertData<TypedMetadata<int32_t>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<int64_t>()) {
            TypedMetadata<int64_t>* meta =
                data->getOrInsertData<TypedMetadata<int64_t>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<float>()) {
            TypedMetadata<float>* meta =
                 data->getOrInsertData<TypedMetadata<float>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<double>()) {
            TypedMetadata<double>* meta =
                data->getOrInsertData<TypedMetadata<double>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<math::Vec3<int32_t>>()) {
            TypedMetadata<math::Vec3<int32_t>>*
                meta = data->getOrInsertData<TypedMetadata<math::Vec3<int32_t>>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<math::Vec3<float>>()) {
            TypedMetadata<math::Vec3<float>>*
                meta = data->getOrInsertData<TypedMetadata<math::Vec3<float>>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else if (type == typeNameAsString<math::Vec3<double>>()) {
            TypedMetadata<math::Vec3<double>>*
                meta = data->getOrInsertData<TypedMetadata<math::Vec3<double>>>(name);
            if (meta) initializer = codegen::LLVMType<uintptr_t>::get(C, meta->value());
        }
        else {
            // grammar guarantees this is unreachable as long as all types are supported
            OPENVDB_THROW(AXCompilerError, "Unsupported $ parameter type \"" + type + "\".");
        }

        if (!initializer) {
            OPENVDB_THROW(AXCompilerError, "Custom data \"" + name + "\" already exists with a "
                "different type.");
        }

        variable->setInitializer(initializer);
        variable->setConstant(true); // is not written to at runtime
    }
}

/// @brief Modifier class that "disables" attribute assignment statements inside of an AST.
class ModifyVolumeAssignments : public ast::Modifier
{
public:

    ModifyVolumeAssignments()
        : mTargetVolAssignmentExpression(0)
        , mCurrentVolAssignmentExpression(0)
        , mVolumesAssigned()
        , mVolumeAssignmentFound(false) {}

    virtual ~ModifyVolumeAssignments() = default;

    virtual ast::Expression* visit(ast::AssignExpression& node) override
    {
        // by default, return nullptr (no replacement). This only changes if we run into an
        // attribute/volume assign

        ast::Expression::UniquePtr replacement = nullptr;

        // extract the "variable" being assigned to

        const ast::Attribute::Ptr attribute =
            std::dynamic_pointer_cast<ast::Attribute>(node.mVariable);

        // in this case, we are assigning to an "attribute" rather than a local variable

        if (attribute) {
            // the "current" volume assignment is the next after the initial one
            if (mCurrentVolAssignmentExpression == mTargetVolAssignmentExpression) {
                // flag that a new assignment has been found
                mVolumeAssignmentFound = true;
                // track the name of the attribute
                mVolumesAssigned.push_back(attribute->mName);
            }
            else {
               replacement.reset(new ast::AttributeValue(attribute->copy()));
            }

            mCurrentVolAssignmentExpression++;
        }

        return replacement.release();
    }


    void restart()
    {
        mVolumeAssignmentFound = false;
        mCurrentVolAssignmentExpression = 0;
    }

    void incrementTargetVolumeAssignment()
    {
        ++mTargetVolAssignmentExpression;
    }

    /// Returns true if an attribute assignment was actually found during AST traversal
    bool volumeAssignmentFound() const
    {
        return mVolumeAssignmentFound;
    }

    /// apends the list of names of volumes that have been assigned to during previous
    /// AST traversals
    void appendVolumesAssigned(std::vector<std::string>& volumes) const
    {
        for (const auto& name : mVolumesAssigned) {
            volumes.push_back(name);
        }
    }

private:

    int mTargetVolAssignmentExpression;
    int mCurrentVolAssignmentExpression;
    std::vector<std::string> mVolumesAssigned;
    bool mVolumeAssignmentFound;
};

/// @brief class that encapsulates blocks of code generated for volume code.
class VolumeCodeBlocks
{
public:

    VolumeCodeBlocks()
        : mBlockFunctionNames()
        , mBlockFunctionAddresses()
        , mVolumesAssigned() {}

    ~VolumeCodeBlocks() = default;

    int numBlocks() const
    {
        return mBlockFunctionNames.size();
    }

    void getVolumesAssigned(std::vector<std::string>& volumeNames) const
    {
        for (const auto& name : mVolumesAssigned) {
            volumeNames.push_back(name);
        }
    }

    void
    compileBlocks(const ast::Tree& syntaxTree,
                  llvm::Module& module,
                  const FunctionOptions& options,
                  codegen::SymbolTable& globals,
                  codegen::FunctionRegistry& functionRegistry,
                  std::vector<std::string>* warnings)
    {
        ModifyVolumeAssignments modifier;
        int volumeCount = 0;

        do {
            openvdb::SharedPtr<ast::Tree> tree(syntaxTree.copy());

            modifier.restart();
            tree->accept(modifier);

            const std::string functionName =
                codegen::VolumeKernel::getDefaultName() + std::to_string(volumeCount);

            codegen::VolumeComputeGenerator codeGenerator(module, options, functionRegistry, warnings);
            codeGenerator.setFunctionName(functionName);
            tree->accept(codeGenerator);

            mBlockFunctionNames.push_back(std::vector<std::string>());
            mBlockFunctionNames.back().emplace_back(functionName);

            // insert any accessed globals into the final global table - different
            // block excutions may access different globals but, as it's all compiled
            // into the same module, we need to track them all

            const codegen::SymbolTable& accessedGlobals = codeGenerator.globals();
            for (const auto& global : accessedGlobals.map()) {
                globals.insert(global.first, global.second);
            }

            // increment "initial"/"base" volume assignment if we found another assignment
            if (modifier.volumeAssignmentFound()) {
                modifier.incrementTargetVolumeAssignment();
            }

            ++volumeCount;
        }
        while (volumeCount < 1000 && modifier.volumeAssignmentFound());

        // mNumBlocks = volumeCount - 1;

        // copy names of volumes which were assigned to
        modifier.appendVolumesAssigned(mVolumesAssigned);
    }

    const std::map<std::string, uint64_t>& functionsForBlock(const int i) const
    {
        assert(mBlockFunctionAddresses.size() > size_t(i));
        return mBlockFunctionAddresses.at(i);
    }

    const std::vector<std::string>&  functionNamesForBlock(const int i) const
    {
        assert(mBlockFunctionAddresses.size() > size_t(i));
        return mBlockFunctionNames.at(i);
    }

    void generateLLVMFunctions(llvm::ExecutionEngine& executionEngine)
    {
        mBlockFunctionAddresses.resize(mVolumesAssigned.size());
        const int numBlocks = mVolumesAssigned.size();

        for (int i = 0; i < numBlocks; i++) {
            for (const std::string& name : mBlockFunctionNames[i]) {
                const uint64_t address = executionEngine.getFunctionAddress(name);
                if (!address) {
                    OPENVDB_THROW(AXCompilerError, "Failed to compile compute function \"" + name + "\"");
                }
                mBlockFunctionAddresses[i][name] = address;
            }
        }
    }

    std::vector<std::map<std::string, uint64_t> > functionsForAllBlocks() const
    {
        return mBlockFunctionAddresses;
    }

private:
    std::vector<std::vector<std::string> > mBlockFunctionNames;
    std::vector<std::map<std::string, uint64_t> > mBlockFunctionAddresses;
    std::vector<std::string> mVolumesAssigned;
};


struct PointDefaultModifier : public openvdb::ax::ast::Modifier
{
    PointDefaultModifier() = default;
    virtual ~PointDefaultModifier() = default;

    const std::set<std::string> vectorDefaults {"P", "v", "N", "Cd"};

    /// @brief  Convert default attribute calls to use vector types for above attributes
    openvdb::ax::ast::Attribute*
    visit(openvdb::ax::ast::Attribute& node) override final
    {
        openvdb::ax::ast::Attribute::UniquePtr replacement;

        if (node.mTypeInferred) {
            // if the user hasn't defined specific type, then allow conversion to vector
            if (vectorDefaults.find(node.mName) != vectorDefaults.end()) {
                replacement.reset(new openvdb::ax::ast::Attribute(node.mName, "vec3s", true));
            }
        }

        return replacement.release();
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
    mFunctionRegistry = codegen::createStandardRegistry(options.mFunctionOptions);
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
    tree->accept(modifier);

    verifyTypedAccesses(*tree);

    // initialize the module and generate LLVM IR

    std::unique_ptr<llvm::Module> module(new llvm::Module("module", *mContext));

    codegen::PointComputeGenerator
        codeGenerator(*module, mCompilerOptions.mFunctionOptions,
            *mFunctionRegistry, warnings);
    tree->accept(codeGenerator);

    // map accesses (always do this prior to optimising as globals may be removed)

    AttributeRegistry::Ptr registry =
        registerAccesses<AttributeRegistry>(codeGenerator.globals(), *tree);

    CustomData::Ptr validCustomData(customData);
    registerExternalGlobals(codeGenerator.globals(), validCustomData, *mContext);

    // as P is accessed specially and not accessed via a global, need to add it to the registry

    if (ast::usesAttribute(*tree, "P")) {
        registry->addData("P", "vec3s", ast::writesToAttribute(*tree, "P"));
    }

    // optimise

    // get module, verify and create execution engine
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

    std::map<std::string, uint64_t> functionMap;

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

    VolumeCodeBlocks volumeCodeBlocks;
    codegen::SymbolTable globals;

    volumeCodeBlocks.compileBlocks(syntaxTree, *module,
        mCompilerOptions.mFunctionOptions, globals, *mFunctionRegistry, warnings);

    // map accesses (always do this prior to optimising as globals may be removed)

    const VolumeRegistry::Ptr registry =
        registerAccesses<VolumeRegistry>(globals, syntaxTree);

    CustomData::Ptr validCustomData(customData);
    registerExternalGlobals(globals, validCustomData, *mContext);

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

    volumeCodeBlocks.generateLLVMFunctions(*executionEngine);
    std::vector<std::string> volumesAssigned;
    volumeCodeBlocks.getVolumesAssigned(volumesAssigned);

    // create final executable object
    VolumeExecutable::Ptr
        executable(new VolumeExecutable(executionEngine, mContext, registry, validCustomData,
            volumeCodeBlocks.functionsForAllBlocks(), volumesAssigned));
    return executable;
}


}
}
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
