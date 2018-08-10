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

/// @authors Nick Avramoussis, Matt Warner, Francisco Gochez, Richard Jones
///

#include "PointComputeGenerator.h"

#include "FunctionRegistry.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/Exceptions.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Pass.h>
#include <llvm/Support/MathExtras.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

const std::string ComputePointFunction::Name = "compute_point";
const std::string ComputePointRangeFunction::Name = "compute_point_range";

const std::array<std::string, ComputePointFunction::N_ARGS> ComputePointFunction::ArgumentKeys =
{
    "custom_data",
    "attribute_set",
    "point_index",
    "attribute_handles",
    "group_handles",
    "leaf_data"
};

PointComputeGenerator::PointComputeGenerator(llvm::Module& module,
                                             CustomData* customData,
                                             const FunctionOptions& options,
                                             FunctionRegistry& functionRegistry,
                                             std::vector<std::string>* const warnings)
    : ComputeGenerator(module, customData, options, functionRegistry, warnings)
    , mLLVMArguments()
    , mAttributeVisitCount(0) {}

void PointComputeGenerator::init(const ast::Tree&)
{
    std::vector<llvm::Type*> argTypes;
    llvmTypesFromSignature<ComputePointFunction::Signature>(mContext, &argTypes);
    assert(argTypes.size() == ComputePointFunction::N_ARGS);
    assert(argTypes.size() == ComputePointFunction::ArgumentKeys.size());

    llvm::FunctionType* computeFunctionType =
        llvm::FunctionType::get(/*Return*/LLVMType<ComputePointFunction::ReturnT>::get(mContext),
                          llvm::ArrayRef<llvm::Type*>(argTypes),
                          /*Variable args*/ false);

    // Function Declarations

    llvm::Function* computePoint =
        llvm::Function::Create(computeFunctionType,
                                llvm::Function::ExternalLinkage,
                                ComputePointFunction::Name,
                                &mModule);

    llvm::Function* computePointRange =
        llvm::Function::Create(computeFunctionType,
                               llvm::Function::ExternalLinkage,
                               ComputePointRangeFunction::Name,
                               &mModule);

    // Check to see if the registration of the function above conflicted
    // If it did, there is already a function called "compute"
    // We should only be making one of these! Something has gone wrong

    if (computePoint->getName() != ComputePointFunction::Name) {
        OPENVDB_THROW(LLVMModuleError, "Function \"" + ComputePointFunction::Name
            + "\" already exists!");
    }

    if (computePointRange->getName() != ComputePointRangeFunction::Name) {
        OPENVDB_THROW(LLVMModuleError, "Function \"" + ComputePointRangeFunction::Name
            + "\" already exists!");
    }

    // Set up arguments for initial entry

    llvm::Function::arg_iterator argIter = computePointRange->arg_begin();
    auto keyIter = ComputePointFunction::ArgumentKeys.cbegin();

    for (; argIter != computePointRange->arg_end(); ++argIter, ++keyIter) {
        if (!mLLVMArguments.insert(*keyIter, llvm::cast<llvm::Value>(argIter))) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + ComputePointRangeFunction::Name
                + "\" has been setup with non-unique argument keys.");
        }
    }

    // Generate the Compute function which simply calls compute_point
    // mPointCount times

    {
        // For the computePointRange function, simply create a for loop which calls
        // compute_point for every point index 0 to mPointCount. The argument types for
        // computePointRange and compute_point are the same, but the third argument for
        // compute_point is the point index rather than the point range

        llvm::BasicBlock* preLoop = llvm::BasicBlock::Create(mContext, "__entry_compute", computePointRange);
        mBuilder.SetInsertPoint(preLoop);

        llvm::Value* indexMinusOne = mBuilder.CreateSub(mLLVMArguments.get("point_index"), mBuilder.getInt64(1));

        llvm::BasicBlock* loop = llvm::BasicBlock::Create(mContext, "__loop_compute", computePointRange);
        mBuilder.CreateBr(loop);
        mBuilder.SetInsertPoint(loop);

        llvm::PHINode* incr = mBuilder.CreatePHI(mBuilder.getInt64Ty(), 2, "i");
        incr->addIncoming(/*start*/mBuilder.getInt64(0), preLoop);

        // Call compute point with incr which will be updated per branch

        std::vector<llvm::Value*> rangeArguments;
        rangeArguments.reserve(ComputePointFunction::ArgumentKeys.size());

        // Map the function arguments. For "point_index", we don't pull in the provided
        // mLLVMArguments "point_index", but instead use the value of incr. incr will correspond
        // to the index of the point being accessed within the ComputePointRangeFunction loop.

        for (const std::string& key : ComputePointFunction::ArgumentKeys) {
            if (key == "point_index") rangeArguments.emplace_back(incr);
            else                      rangeArguments.emplace_back(mLLVMArguments.get(key));
        }

        mBuilder.CreateCall(computePoint, rangeArguments);

        llvm::Value* next = mBuilder.CreateAdd(incr, mBuilder.getInt64(1), "nextval");

        llvm::Value* endCondition = mBuilder.CreateICmpULT(incr, indexMinusOne, "endcond");
        llvm::BasicBlock* loopEnd = mBuilder.GetInsertBlock();

        llvm::BasicBlock* postLoop = llvm::BasicBlock::Create(mContext, "__post_loop_compute", computePointRange);
        mBuilder.CreateCondBr(endCondition, loop, postLoop);
        mBuilder.SetInsertPoint(postLoop);
        incr->addIncoming(next, loopEnd);

        mBuilder.CreateRetVoid();
        mBuilder.ClearInsertionPoint();
    }

    // Now generate the code for compute_point by setting up a new block and
    // continuing the visit to all nodes

    argIter = computePoint->arg_begin();
    keyIter = ComputePointFunction::ArgumentKeys.cbegin();

    for (; argIter != computePoint->arg_end(); ++argIter, ++keyIter) {
        mLLVMArguments.replace(*keyIter, llvm::cast<llvm::Value>(argIter));
    }

    mBlocks.push(llvm::BasicBlock::Create(mContext, "__entry_compute_point", computePoint));
    mBuilder.SetInsertPoint(mBlocks.top());
    mCurrentBlock = 1;

    // Set the base code generator function to the compute point function

    mFunction = computePoint;
}

void PointComputeGenerator::visit(const ast::AssignExpression& node)
{
    // Enum of supported assignments within the PointComputeGenerator

    enum AssignmentType
    {
        UNSUPPORTED = 0,
        STRING_EQ_STRING,
        ARRAY_EQ_ARRAY,
        SCALAR_EQ_ARRAY,
        ARRAY_EQ_SCALAR,
        SCALAR_EQ_SCALAR
    };

    // if not assigning to an attribute, use the base implementation
    if (mAttributeVisitCount == 0) {
        ComputeGenerator::visit(node);
        return;
    }

    --mAttributeVisitCount;

    // values are not loaded. rhs is always a pointer to a scalar or array,
    // where as the lhs is always void* to the attribute handle or the leaf data

    llvm::Value* handlePtr = mValues.top(); mValues.pop(); // lhs
    llvm::Value* rhs = mValues.top(); mValues.pop();

    assert(rhs && rhs->getType()->isPointerTy() &&
           "Right Hand Size input to AssignExpression is not a pointer type.");
    assert(handlePtr && handlePtr->getType()->isPointerTy() &&
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

    const std::string& type = attribute->mType;
    const bool usingPosition = (attribute->mName == "P");

    // attribute should already exist
    assert(usingPosition || this->globals().exists(getGlobalAttributeAccess(attribute->mName, type)));

    const bool lhsIsString = type == "string";

    llvm::Type* rhsType = rhs->getType()->getContainedType(0);
    llvm::Type* lhsType = llvmTypeFromName(type, mContext);

    // convert rhs to match lhs for all supported assignments:
    // (scalar=scalar, vector=vector, scalar=vector, vector=scalar etc)

    AssignmentType assignmentType = UNSUPPORTED;
    if (isCharType(lhsType, mContext) && isCharType(rhsType, mContext)) {
        assignmentType = STRING_EQ_STRING;
    }
    else if (!(isCharType(lhsType, mContext) && isCharType(rhsType, mContext))) {
        const bool lhsIsArray = isArrayType(lhsType);
        const bool rhsIsArray = isArrayType(rhsType);
        if (lhsIsArray && rhsIsArray)        assignmentType = ARRAY_EQ_ARRAY;
        else if (!lhsIsArray && rhsIsArray)  assignmentType = SCALAR_EQ_ARRAY;
        else if (lhsIsArray && !rhsIsArray)  assignmentType = ARRAY_EQ_SCALAR;
        else                                 assignmentType = SCALAR_EQ_SCALAR;
    }

    switch (assignmentType) {
        case STRING_EQ_STRING : {
            // rhs is a pointer to start of char buffer which is already the correct
            // argument format for set point string
            break;
        }
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
    std::vector<llvm::Value*> argumentValues;
    argumentValues.reserve(lhsIsString ? 4 : 3);

    // push back remaining argument types and values
    argumentValues.emplace_back(handlePtr);
    argumentValues.emplace_back(mLLVMArguments.get("point_index")); // point index
    argumentValues.emplace_back(rhs);

    if (lhsIsString) {
        argumentValues.emplace_back(mLLVMArguments.get("leaf_data")); // new point data
    }

    if (usingPosition) {
        const FunctionBase::Ptr function = this->getFunction("setpointpws", mOptions, true);
        function->execute(argumentValues, mLLVMArguments.map(), mBuilder, mModule);
    }
    else {
        const FunctionBase::Ptr function = this->getFunction("setattribute", mOptions, true);
        function->execute(argumentValues, mLLVMArguments.map(), mBuilder, mModule);
    }
}

void PointComputeGenerator::visit(const ast::Crement& node)
{
    // if not visiting an attribute, use base implementation
    if (mAttributeVisitCount == 0) {
        ComputeGenerator::visit(node);
        return;
    }

    --mAttributeVisitCount;

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

    // decide whether adding or subtracting
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

    assert(node.mVariable);

    std::vector<llvm::Value*> argumentValues;
    argumentValues.reserve(3);

    argumentValues.emplace_back(lhs);
    argumentValues.emplace_back(mLLVMArguments.get("point_index"));
    argumentValues.emplace_back(rhs);

    // @TODO: if supporting vector crement, reenable this
    // const ast::Attribute* const attribute =
    //     static_cast<const ast::Attribute* const>(node.mVariable.get());
    // assert(attribute);
    // if (attribute->mName == "P") {
    //     const FunctionBase::Ptr function = getFunctionFromRegistry("__setpointpws", mOptions);
    //     function->execute(argumentValues, mLLVMArguments.map(), mBuilder, mModule);
    // } else {
    const FunctionBase::Ptr function = this->getFunction("setattribute", mOptions, true);
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

void PointComputeGenerator::visit(const ast::FunctionCall& node)
{
    assert(node.mArguments.get() && ("Uninitialized expression list for " +
           node.mFunction).c_str());

    const FunctionBase::Ptr function =
        this->getFunction(node.mFunction, mOptions, /*no internal access*/false);
    assert(function);

    if (function->context() & FunctionBase::Base) {
        ComputeGenerator::visit(node);
        return;
    }

    if (!(function->context() & FunctionBase::Point)) {
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

void PointComputeGenerator::visit(const ast::Attribute& node)
{
    if (node.mName == "P") {
        // if accessing position the ptr we push back is actually to the leaf_data
        llvm::Value* leafDataPtr = mLLVMArguments.get("leaf_data");
        ++mAttributeVisitCount;

        mValues.push(leafDataPtr);
    }
    else {
        // Visiting an attribute - get the attribute handle out of a vector of void pointers
        // mLLVMArguments.get("attribute_handles") is a void pointer to a vector of void
        // pointers (void**)

        // insert the attribute into the map of global variables and get a unique global representing
        // the location which will hold the attribute handle offset.

        const std::string globalName = getGlobalAttributeAccess(node.mName, node.mType);

        llvm::Value* index = llvm::cast<llvm::GlobalVariable>
            (mModule.getOrInsertGlobal(globalName, LLVMType<int64_t>::get(mContext)));
        this->globals().insert(globalName, index);

        // index into the void* array of handles and load the value.
        // The result is a loaded void* value

        index = mBuilder.CreateLoad(index);
        llvm::Value* handlePtr = mBuilder.CreateGEP(mLLVMArguments.get("attribute_handles"), index);
        handlePtr = mBuilder.CreateLoad(handlePtr);

        // indicate the next value is an attribute

        ++mAttributeVisitCount;

        // push back the handle pointer

        mValues.push(handlePtr);
    }

}

void PointComputeGenerator::visit(const ast::AttributeValue& node)
{
    assert(mAttributeVisitCount != 0 &&
        "Expected attribute is marked as a local");
    assert(node.mAttribute &&
        "No attribute data initialized for attribute value");

    // get the values and remove the attribute flag

    llvm::Value* handlePtr = mValues.top(); mValues.pop();
    --mAttributeVisitCount;

    const std::string& name = node.mAttribute->mName;
    const std::string& type = node.mAttribute->mType;

    const bool usingPosition = name == "P";

    // attribute should have already been inserted - see visit(ast::Attribute)

    assert(usingPosition || this->globals().exists(getGlobalAttributeAccess(name, type)));

    llvm::Type* returnType = llvmTypeFromName(type, mContext);

    llvm::Value* returnValue = nullptr;
    std::vector<llvm::Value*> args;

    const bool usingString(!usingPosition && type == "string");
    if (usingString) {
        const FunctionBase::Ptr function = this->getFunction("strattribsize", mOptions, true);
        llvm::Value* size =
            function->execute({handlePtr, mLLVMArguments.get("point_index"), mLLVMArguments.get("leaf_data")},
                mLLVMArguments.map(), mBuilder, mModule, nullptr, true);

        returnValue = mBuilder.CreateAlloca(returnType, size);
        args.reserve(4);
    }
    else {
        returnValue = mBuilder.CreateAlloca(returnType);
        args.reserve(3);
    }

    args.emplace_back(handlePtr);
    args.emplace_back(mLLVMArguments.get("point_index"));
    args.emplace_back(returnValue);

    if (usingString) args.emplace_back(mLLVMArguments.get("leaf_data"));

    if (usingPosition) {
        const FunctionBase::Ptr function = this->getFunction("getpointpws", mOptions, true);
        function->execute(args, mLLVMArguments.map(), mBuilder, mModule, nullptr, /*add output args*/false);
    }
    else {
        const FunctionBase::Ptr function = this->getFunction("getattribute", mOptions, true);
        function->execute(args, mLLVMArguments.map(), mBuilder, mModule, nullptr, /*add output args*/false);
    }

    mValues.push(returnValue);
}

}
}
}
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
