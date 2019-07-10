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

/// @authors Nick Avramoussis
///

#include "VolumeComputeGenerator.h"

#include "FunctionRegistry.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/ast/Scanners.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

const std::array<std::string, VolumeKernel::N_ARGS>&
VolumeKernel::argumentKeys()
{
    static const std::array<std::string, VolumeKernel::N_ARGS> arguments = {
        "custom_data",
        "coord_is",
        "coord_ws",
        "accessors",
        "transforms",
        "write_index",
        "write_acccessor"
    };

    return arguments;
}

std::string VolumeKernel::getDefaultName() { return "compute_voxel"; }


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


VolumeComputeGenerator::VolumeComputeGenerator(llvm::Module& module,
                                               const FunctionOptions& options,
                                               FunctionRegistry& functionRegistry,
                                               std::vector<std::string>* const warnings)
    : ComputeGenerator(module, options, functionRegistry, warnings) {}

AttributeRegistry::Ptr VolumeComputeGenerator::generate(const ast::Tree& tree)
{
    // Override the ComputeGenerators default init() with the custom
    // functions requires for Volume execution

    using FunctionSignatureT = FunctionSignature<VolumeKernel::Signature>;

    // Use the function signature type to generate the llvm function

    const FunctionSignatureT::Ptr volumeKernelSignature =
        FunctionSignatureT::create(nullptr, VolumeKernel::getDefaultName(), 0);

    // Set the base code generator function to the compute voxel function

    mFunction = volumeKernelSignature->toLLVMFunction(mModule);

    // Set up arguments for initial entry

    llvm::Function::arg_iterator argIter = mFunction->arg_begin();
    const auto arguments = VolumeKernel::argumentKeys();
    auto keyIter = arguments.cbegin();

    for (; argIter != mFunction->arg_end(); ++argIter, ++keyIter) {
        if (!mLLVMArguments.insert(*keyIter, llvm::cast<llvm::Value>(argIter))) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + VolumeKernel::getDefaultName()
                + "\" has been setup with non-unique argument keys.");
        }
    }

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(mContext,
        "entry_" + VolumeKernel::getDefaultName(), mFunction);
    mBuilder.SetInsertPoint(entry);

    // build the attribute registry

    AttributeRegistry::Ptr registry = AttributeRegistry::create(tree);

    // Visit all attributes and allocate them in local IR memory - assumes attributes
    // have been verified by the ax compiler
    // @note  Call all attribute allocs at the start of this block so that llvm folds
    // them into the function prologue (as a static allocation)

    SymbolTable* localTable = this->mSymbolTables.getOrInsert(1);

    // run allocations and update the symbol table

    for (const AttributeRegistry::AccessData& data : registry->data()) {
        llvm::Value* value = mBuilder.CreateAlloca(llvmTypeFromToken(data.type(), mContext));
        assert(llvm::cast<llvm::AllocaInst>(value)->isStaticAlloca());
        localTable->insert(data.tokenname(), value);
    }

    // insert getters for read variables

    for (const AttributeRegistry::AccessData& data : registry->data()) {
        if (!data.reads()) continue;
        const std::string token = data.tokenname();
        this->getAccessorValue(token, localTable->get(token));
    }

    // full code generation

    this->traverse(&tree);

    // insert set code

    std::vector<const AttributeRegistry::AccessData*> write;
    for (const AttributeRegistry::AccessData& access : registry->data()) {
        if (access.writes()) write.emplace_back(&access);
    }
    if (write.empty()) return registry;

    // Cache the basic blocks which have been created as we will create
    // new branches below
    std::vector<llvm::BasicBlock*> blocks;
    for (auto block = mFunction->begin(); block != mFunction->end(); ++block) {
        blocks.emplace_back(&*block);
    }

    // insert set voxel calls

    for (auto& block : blocks) {

        // Only inset set calls if theres a valid return instruction in this block

        llvm::Instruction* inst = block->getTerminator();
        if (!inst || !llvm::isa<llvm::ReturnInst>(inst)) continue;
        // remove the old return statement (we'll point to the final return jump)
        inst->eraseFromParent();

        // Set builder to the end of this block

        mBuilder.SetInsertPoint(&(*block));

        for (const AttributeRegistry::AccessData* access : write) {

            const std::string token = access->tokenname();
            llvm::Value* value = localTable->get(token);
            // Expected to be used more than one (i.e. should never be zero)
            assert(value->hasNUsesOrMore(1));

            // Check to see if this value is still being used - it may have
            // been cleaned up due to returns. If there's only one use, it's
            // the original get of this attribute.
            if (value->hasOneUse()) {
                // @todo  The original get can also be optimized out in this case
                // this->globals().remove(variable.first);
                // mModule.getGlobalVariable(variable.first)->eraseFromParent();
                continue;
            }

            llvm::Value* accessIndex = mLLVMArguments.get("write_index");
            llvm::Value* registeredIndex = llvm::cast<llvm::GlobalVariable>
                (mModule.getOrInsertGlobal(token, LLVMType<int64_t>::get(mContext)));
            registeredIndex = mBuilder.CreateLoad(registeredIndex);

            llvm::Value* result = mBuilder.CreateICmpEQ(accessIndex, registeredIndex);
            result = boolComparison(result, mBuilder);

            llvm::BasicBlock* thenBlock =
                llvm::BasicBlock::Create(mContext, "post_assign " + token, mFunction);
            llvm::BasicBlock* continueBlock =
                llvm::BasicBlock::Create(mContext, "post_continue", mFunction);

            mBuilder.CreateCondBr(result, thenBlock, continueBlock);
            mBuilder.SetInsertPoint(thenBlock);

            // get the accessor
            llvm::Value* accessor = mLLVMArguments.get("write_acccessor");

            // load the result if its not an array
            if (!value->getType()->getPointerElementType()->isArrayTy()) {
                value = mBuilder.CreateLoad(value);
            }

            // construct function arguments
            const std::vector<llvm::Value*> argumentValues {
                accessor, mLLVMArguments.get("coord_is"), value
            };

            const FunctionBase::Ptr function = this->getFunction("setvoxel", mOptions, true);
            function->execute(argumentValues, mLLVMArguments.map(), mBuilder);

            mBuilder.CreateBr(continueBlock);
            mBuilder.SetInsertPoint(continueBlock);
        }

        mBuilder.CreateRetVoid();
    }

    return registry;
}

bool VolumeComputeGenerator::visit(const ast::Attribute* node)
{
    const std::string globalName =
        getGlobalAttributeAccess(node->name(), node->typestr());
    SymbolTable* localTable = this->mSymbolTables.getOrInsert(1);
    llvm::Value* value = localTable->get(globalName);
    assert(value);
    mValues.push(value);
    return true;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void VolumeComputeGenerator::getAccessorValue(const std::string& globalName, llvm::Value* location)
{
    std::string name, type;
    isGlobalAttributeAccess(globalName, name, type);

    llvm::Value* registeredIndex = llvm::cast<llvm::GlobalVariable>
        (mModule.getOrInsertGlobal(globalName, LLVMType<int64_t>::get(mContext)));
    this->globals().insert(globalName, registeredIndex);
    registeredIndex = mBuilder.CreateLoad(registeredIndex);

    // index into the void* array of handles and load the value.
    // The result is a loaded void* value

    llvm::Value* accessorPtr = mBuilder.CreateGEP(mLLVMArguments.get("accessors"), registeredIndex);
    llvm::Value* transformPtr = mBuilder.CreateGEP(mLLVMArguments.get("transforms"), registeredIndex);
    llvm::Value* accessor = mBuilder.CreateLoad(accessorPtr);
    llvm::Value* transform = mBuilder.CreateLoad(transformPtr);

    const std::vector<llvm::Value*> args {
        accessor, transform, mLLVMArguments.get("coord_ws"), location
    };

    const FunctionBase::Ptr function = this->getFunction("getvoxel", mOptions, true);
    function->execute(args, mLLVMArguments.map(), mBuilder, nullptr, /*add output args*/false);
}

llvm::Value* VolumeComputeGenerator::accessorHandleFromToken(const std::string& globalName)
{
    // Visiting an "attribute" - get the volume accessor out of a vector of void pointers
    // mAttributeHandles is a void pointer to a vector of void pointers (void**)

    llvm::Value* registeredIndex = llvm::cast<llvm::GlobalVariable>
        (mModule.getOrInsertGlobal(globalName, LLVMType<int64_t>::get(mContext)));
    this->globals().insert(globalName, registeredIndex);

    registeredIndex = mBuilder.CreateLoad(registeredIndex);

    // index into the void* array of handles and load the value.
    // The result is a loaded void* value

    llvm::Value* accessorPtr =
        mBuilder.CreateGEP(mLLVMArguments.get("accessors"), registeredIndex);

    // return loaded void** = void*
    return mBuilder.CreateLoad(accessorPtr);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


}
}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
