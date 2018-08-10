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

/// @authors Nick Avramoussis
///

#include "VolumeComputeGenerator.h"

#include "FunctionRegistry.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/Exceptions.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

const std::string ComputeVolumeFunction::DefaultName = "compute_volume";

const std::array<std::string, ComputeVolumeFunction::N_ARGS> ComputeVolumeFunction::ArgumentKeys =
{
    "custom_data",
    "coord_is",
    "coord_ws",
    "accessors",
    "transforms"
};

VolumeComputeGenerator::VolumeComputeGenerator(llvm::Module& module,
                                               CustomData* const customData,
                                               const FunctionOptions& options,
                                               FunctionRegistry& functionRegistry,
                                               std::vector<std::string>* const warnings,
                                               const std::string& functionName)
    : ComputeGenerator(module, customData, options, functionRegistry, warnings)
    , mLLVMArguments()
    , mVolumeVisitCount(0)
    , mFunctionName(functionName) {}

void VolumeComputeGenerator::init(const ast::Tree&)
{
    // Override the ComputeGenerators default init() with the custom
    // functions requires for Volume execution

    std::vector<llvm::Type*> argTypes;
    llvmTypesFromSignature<ComputeVolumeFunction::Signature>(mContext, &argTypes);
    assert(argTypes.size() == ComputeVolumeFunction::N_ARGS);
    assert(argTypes.size() == ComputeVolumeFunction::ArgumentKeys.size());

    llvm::FunctionType* computeFunctionType =
        llvm::FunctionType::get(/*Return*/LLVMType<ComputeVolumeFunction::ReturnT>::get(mContext),
                          llvm::ArrayRef<llvm::Type*>(argTypes),
                          /*Variable args*/ false);

    // Function Declaration

    llvm::Function* computeVolume =
        llvm::Function::Create(computeFunctionType,
                                llvm::Function::ExternalLinkage,
                                mFunctionName,
                                &mModule);

    // Check to see if the registration of the function above conflicted
    // If it did, there is already a function called "compute"
    // We should only be making one of these! Something has gone wrong

    if (computeVolume->getName() != mFunctionName) {
        OPENVDB_THROW(LLVMModuleError, "Function \"" + mFunctionName +
            + "\" already exists!");
    }

    // Set up arguments for initial entry

    llvm::Function::arg_iterator argIter = computeVolume->arg_begin();
    auto keyIter = ComputeVolumeFunction::ArgumentKeys.cbegin();

    for (; argIter != computeVolume->arg_end(); ++argIter, ++keyIter) {
        if (!mLLVMArguments.insert(*keyIter, llvm::cast<llvm::Value>(argIter))) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + mFunctionName
                + "\" has been setup with non-unique argument keys.");
        }
    }

    mBlocks.push(llvm::BasicBlock::Create(mContext, "__entry_compute_volume", computeVolume));
    mBuilder.SetInsertPoint(mBlocks.top());
    mCurrentBlock = 1;

    // Set the base code generator function to the compute point function

    mFunction = computeVolume;
}

void VolumeComputeGenerator::visit(const ast::AssignExpression& node)
{
    // Enum of supported assignments within the VolumeComputeGenerator

    enum AssignmentType
    {
        UNSUPPORTED = 0,
        ARRAY_EQ_ARRAY,
        SCALAR_EQ_ARRAY,
        ARRAY_EQ_SCALAR,
        SCALAR_EQ_SCALAR
    };

    if (mVolumeVisitCount == 0) {
        ComputeGenerator::visit(node);
        return;
    }

    --mVolumeVisitCount;

    // values are not loaded. rhs is always a pointer to a scalar or array,
    // where as the lhs is always void* to the volume accessor

    llvm::Value* accessorPtr = mValues.top(); mValues.pop(); // lhs
    llvm::Value* rhs = mValues.top(); mValues.pop();

    assert(rhs && rhs->getType()->isPointerTy() &&
           "Right Hand Size input to AssignExpression is not a pointer type.");
    assert(accessorPtr && accessorPtr->getType()->isPointerTy() &&
           "Left Hand Size input to AssignExpression is not a pointer type.");

    // Push the original RHS value back onto stack to allow for multiple
    // assignment statements to be chained together

    mValues.push(rhs);

    // LHS is always a pointer to an attribute here. Find the value type requested
    // from the AST node

    assert(node.mVariable);
    const ast::Attribute* const attribute =
        static_cast<const ast::Attribute* const>(node.mVariable.get());
    assert(attribute);

    // mark the attribute in the engine data as writeable. It should already exist
    // See visit(ast::Attribute)

    const std::string& type = attribute->mType;

    // attribute should already exist

    assert(this->globals().exists(getGlobalAttributeAccess(attribute->mName, type)));

    llvm::Type* rhsType = rhs->getType()->getContainedType(0);
    llvm::Type* lhsType = llvmTypeFromName(type, mContext);

    // convert rhs to match lhs for all supported assignments:
    // (scalar=scalar, vector=vector, scalar=vector, vector=scalar etc)
    // @note assume string volumes don't exist
    // @note target could be a local string from a volume (@todo implicit cast)

    const bool lhsIsArray = isArrayType(lhsType);
    const bool lhsIsScalar = isScalarType(lhsType);
    const bool rhsIsArray = isArrayType(rhsType);
    const bool rhsIsScalar = isScalarType(rhsType);

    AssignmentType assignmentType = UNSUPPORTED;
    if (lhsIsArray && rhsIsArray)         assignmentType = ARRAY_EQ_ARRAY;
    else if (lhsIsScalar && rhsIsArray)   assignmentType = SCALAR_EQ_ARRAY;
    else if (lhsIsArray && rhsIsScalar)   assignmentType = ARRAY_EQ_SCALAR;
    else if (lhsIsScalar && rhsIsScalar)  assignmentType = SCALAR_EQ_SCALAR;

    switch (assignmentType) {
        case ARRAY_EQ_ARRAY : {
            const size_t lhsSize = lhsType->getArrayNumElements();
            const size_t rhsSize = rhsType->getArrayNumElements();
            if (lhsSize != rhsSize) {
                OPENVDB_THROW(LLVMArrayError, "Unable to assign vector/array "
                    "attributes with mismatching sizes");
            }

            // vector = vector - convert rhs to matching lhs type if necessary
            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            rhs = arrayCast(rhs, lhsElementType, mBuilder);
            break;
        }
        case SCALAR_EQ_ARRAY : {
            // take the first value of the array
            rhs = arrayIndexUnpack(rhs, 0, mBuilder);
            rhs = mBuilder.CreateLoad(rhs);
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);
            break;
        }
        case ARRAY_EQ_SCALAR : {
            // convert rhs to a vector of the same value
            rhs = mBuilder.CreateLoad(rhs);
            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            rhs = arithmeticConversion(rhs, lhsElementType, mBuilder);
            rhs = arrayPack(rhs, mBuilder, lhsType->getArrayNumElements());
            break;
        }
        case SCALAR_EQ_SCALAR : {
            // load and implicit conversion
            rhs = mBuilder.CreateLoad(rhs);
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);
            break;
        }
        default : {
            OPENVDB_THROW(LLVMCastError, "Unsupported implicit cast in assignment.");
        }
    }

    // construct function arguments

    const std::vector<llvm::Value*> argumentValues {
        accessorPtr, mLLVMArguments.get("coord_is"), rhs
    };

    const FunctionBase::Ptr function = this->getFunction("setvoxel", mOptions, true);
    function->execute(argumentValues, mLLVMArguments.map(), mBuilder, mModule);
}

void VolumeComputeGenerator::visit(const ast::Crement& node)
{
    if (mVolumeVisitCount == 0) {
        ComputeGenerator::visit(node);
        return;
    }

    --mVolumeVisitCount;

    llvm::Value* rhs = mValues.top(); mValues.pop();
    llvm::Value* lhs = mValues.top(); mValues.pop();

    rhs = mBuilder.CreateLoad(rhs);
    llvm::Type* type = rhs->getType();

    // if we are post incrementing (i.e. i++) store the current
    // value to push back into the stack afterwards

    llvm::Value* temp = nullptr;
    if (node.mPost) {
        temp = mBuilder.CreateAlloca(type);
        mBuilder.CreateStore(rhs, temp);
    }

    // Decide whether adding or subtracting
    // (We use the add instruction in both cases!)

    int oneOrMinusOne;
    if (node.mOperation == ast::Crement::Increment) oneOrMinusOne = 1;
    else if (node.mOperation == ast::Crement::Decrement) oneOrMinusOne = -1;
    else OPENVDB_THROW(LLVMTokenError, "Unrecognised crement operation token");

    // add or subtract one from the variable

    if (!isCharType(type, mContext) && type->isIntegerTy() && !type->isIntegerTy(1)) {
        rhs = mBuilder.CreateAdd(rhs, llvm::ConstantInt::get(type, oneOrMinusOne));
    }
    else if (!isCharType(type, mContext) && type->isFloatingPointTy()) {
        rhs = mBuilder.CreateFAdd(rhs, llvm::ConstantFP::get(type, oneOrMinusOne));
    }
    else {
        OPENVDB_THROW(LLVMTypeError, "Variable \"" + node.mVariable->mName +
            "\" is an unsupported type for crement. Must be scalar.");
    }

    const std::vector<llvm::Value*> argumentValues {
        lhs, mLLVMArguments.get("coord_is"), rhs
    };

    const FunctionBase::Ptr function = this->getFunction("setvoxel", mOptions, true);
    function->execute(argumentValues, mLLVMArguments.map(), mBuilder, mModule);

    // decide what to put on the expression stack

    if (node.mPost) {
        // post-increment: put the original value on the expression stack
        mValues.push(temp);
    }
    else {
        // pre-increment: put the incremented value on the expression stack
        lhs = mBuilder.CreateAlloca(type);
        mBuilder.CreateStore(rhs, lhs);
        mValues.push(lhs);
    }
}

void VolumeComputeGenerator::visit(const ast::FunctionCall& node)
{

    assert(node.mArguments.get() && ("Uninitialized expression list for " +
           node.mFunction).c_str());

    const FunctionBase::Ptr function = this->getFunction(node.mFunction, mOptions, /*no internal access*/false);
    assert(function);

    if (function->context() & FunctionBase::Base) {
        ComputeGenerator::visit(node);
        return;
    }

    if (!(function->context() & FunctionBase::Volume)) {
        OPENVDB_THROW(LLVMContextError, "\"" + node.mFunction +
            "\" called within an invalid context");
    }

    const size_t args = node.mArguments->mList.size();

    std::vector<llvm::Value*> arguments;
    argumentsFromStack(mValues, args, arguments);
    parseDefaultArgumentState(arguments, mBuilder);

    std::vector<llvm::Value*> results;
    llvm::Value* result = function->execute(arguments, mLLVMArguments.map(), mBuilder, mModule, &results);
    llvm::Type* resultType = result->getType();

    if (resultType != LLVMType<void>::get(mContext)) {
        // only required to allocate new data for the result type if its NOT a pointer
        if (!resultType->isPointerTy()) {
            llvm::Value* resultStore = mBuilder.CreateAlloca(resultType);
            mBuilder.CreateStore(result, resultStore);
            result = resultStore;
        }
        mValues.push(result);
    }

    for (auto& v : results) mValues.push(v);
}

void VolumeComputeGenerator::visit(const ast::Attribute& node)
{
    // Visiting an "attribute" - get the volume accessor out of a vector of void pointers
    // mAttributeHandles is a void pointer to a vector of void pointers (void**)

    const std::string globalName = getGlobalAttributeAccess(node.mName, node.mType);

    llvm::Value* registeredIndex = llvm::cast<llvm::GlobalVariable>
        (mModule.getOrInsertGlobal(globalName, LLVMType<int64_t>::get(mContext)));
    this->globals().insert(globalName, registeredIndex);

    registeredIndex = mBuilder.CreateLoad(registeredIndex);

    // index into the void* array of handles and load the value.
    // The result is a loaded void* value

    llvm::Value* accessorPtr = mBuilder.CreateGEP(mLLVMArguments.get("accessors"), registeredIndex);
    accessorPtr = mBuilder.CreateLoad(accessorPtr);

    // indicate the next value is an attribute

    ++mVolumeVisitCount;

    // push back the handle pointer
    mValues.push(accessorPtr);
}

void VolumeComputeGenerator::visit(const ast::AttributeValue& node)
{
    assert(mVolumeVisitCount != 0 &&
        "Expected attribute is marked as a local");
    assert(node.mAttribute &&
        "No attribute data initialized for attribute value");

    // get the values and remove the attribute flag

    --mVolumeVisitCount;

    llvm::Value* accessorValue = mValues.top(); mValues.pop();

    // volume should have already been inserted - see visit(ast::Attribute)

    const std::string globalName =
        getGlobalAttributeAccess(node.mAttribute->mName, node.mAttribute->mType);
    assert(this->globals().exists(globalName));

    llvm::Value* registeredIndex = llvm::cast<llvm::GlobalVariable>
        (mModule.getOrInsertGlobal(globalName, LLVMType<int64_t>::get(mContext)));

    registeredIndex = mBuilder.CreateLoad(registeredIndex);

    // retrieve volume transform

    llvm::Value* transformPtr = mBuilder.CreateGEP(mLLVMArguments.get("transforms"), registeredIndex);
    llvm::Value* transform = mBuilder.CreateLoad(transformPtr);

    llvm::Type* returnType = llvmTypeFromName(node.mAttribute->mType, mContext);
    llvm::Value* returnValue = mBuilder.CreateAlloca(returnType);

    const std::vector<llvm::Value*> args {
        accessorValue, transform, mLLVMArguments.get("coord_ws"), returnValue
    };

    const FunctionBase::Ptr function = this->getFunction("getvoxel", mOptions, true);
    function->execute(args, mLLVMArguments.map(), mBuilder, mModule, nullptr, /*add output args*/false);

    mValues.push(returnValue);
}


}
}
}
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
