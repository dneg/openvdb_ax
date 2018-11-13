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

///
/// @authors Nick Avramoussis, Matt Warner, Francisco Gochez, Richard Jones
///

#include "ComputeGenerator.h"

#include "FunctionRegistry.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/Exceptions.h>

#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Pass.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Transforms/Utils/BuildLibCalls.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {


const std::array<std::string, ComputeKernel::N_ARGS>&
ComputeKernel::getArgumentKeys()
{
    static const std::array<std::string, ComputeKernel::N_ARGS> arguments = {
        "custom_data"
    };

    return arguments;
}

std::string ComputeKernel::getDefaultName() { return "compute"; }


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


ComputeGenerator::ComputeGenerator(llvm::Module& module,
                                   const FunctionOptions& options,
                                   FunctionRegistry& functionRegistry,
                                   std::vector<std::string>* const warnings)
    : mModule(module)
    , mContext(module.getContext())
    , mBuilder(llvm::IRBuilder<>(module.getContext()))
    , mBlocks()
    , mReturnBlocks()
    , mContinueBlocks()
    , mCurrentBlock(1)
    , mValues()
    , mSymbolTables()
    , mWarnings(warnings)
    , mFunction(nullptr)
    , mLLVMArguments()
    , mOptions(options)
    , mTargetLibInfoImpl(new llvm::TargetLibraryInfoImpl(llvm::Triple(mModule.getTargetTriple())))
    , mFunctionRegistry(functionRegistry) {}

void ComputeGenerator::init(const ast::Tree&)
{
    // Initialise a default function body which returns void and accepts
    // a custom data pointer as an argument

    using FunctionSignatureT = FunctionSignature<ComputeKernel::Signature>;

    // Use the function signature type to generate the llvm function

    const FunctionSignatureT::Ptr computeKernel =
        FunctionSignatureT::create(nullptr, ComputeKernel::getDefaultName());

    // Set the base code generator function to the compute voxel function

    mFunction = computeKernel->toLLVMFunction(mModule);

    // Set up arguments for initial entry

    llvm::Function::arg_iterator argIter = mFunction->arg_begin();
    const auto arguments = ComputeKernel::getArgumentKeys();
    auto keyIter = arguments.cbegin();

    for (; argIter != mFunction->arg_end(); ++argIter, ++keyIter) {
        if (!mLLVMArguments.insert(*keyIter, llvm::cast<llvm::Value>(argIter))) {
            OPENVDB_THROW(LLVMFunctionError, "Function \"" + ComputeKernel::getDefaultName()
                + "\" has been setup with non-unique argument keys.");
        }
    }

    mBlocks.push(llvm::BasicBlock::Create(mContext,
        "entry_" + ComputeKernel::getDefaultName(), mFunction));
    mBuilder.SetInsertPoint(mBlocks.top());
}

void ComputeGenerator::visit(const ast::Block& node)
{
    if (mBlocks.size() > 1) {
        mBuilder.CreateBr(mContinueBlocks.top());
        mContinueBlocks.pop();

        mBuilder.SetInsertPoint(mBlocks.top());
        mBlocks.pop();

        // remove the symbol table created in this branch
        mSymbolTables.erase(mCurrentBlock);
        mCurrentBlock = mBlocks.size();
    }
}

void ComputeGenerator::visit(const ast::ConditionalStatement& node)
{
    llvm::Value* condition = mValues.top(); mValues.pop();

    condition = mBuilder.CreateLoad(condition);
    condition = boolComparison(condition, mBuilder);

    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(mContext, "then", mFunction);
    llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(mContext, "else", mFunction);
    llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(mContext, "continue", mFunction);

    mBuilder.CreateCondBr(condition, thenBlock, elseBlock);

    mBlocks.push(continueBlock);
    mBlocks.push(elseBlock);

    mContinueBlocks.push(continueBlock);
    mContinueBlocks.push(continueBlock);

    mBuilder.SetInsertPoint(thenBlock);

    mCurrentBlock = mBlocks.size();
}

void ComputeGenerator::visit(const ast::BinaryOperator& node)
{
    // Enum of supported operations

    enum OperandTypes
    {
        UNSUPPORTED = 0,
        STRING_OP_STRING,
        ARRAY_OP_ARRAY,
        SCALAR_OP_ARRAY,
        ARRAY_OP_SCALAR,
        SCALAR_OP_SCALAR
    };

    llvm::Value* ptrToRhsValue = mValues.top(); mValues.pop();
    llvm::Value* ptrToLhsValue = mValues.top(); mValues.pop();

    assert(ptrToRhsValue && ptrToRhsValue->getType()->isPointerTy() &&
        "Right Hand Size input to BinaryOperator is not a pointer type.");
    assert(ptrToLhsValue && ptrToLhsValue->getType()->isPointerTy() &&
        "Left Hand Size input to BinaryOperator is not a pointer type.");

    llvm::Type* lhsType = ptrToLhsValue->getType()->getContainedType(0);
    llvm::Type* rhsType = ptrToRhsValue->getType()->getContainedType(0);

    // convert rhs to match lhs for all supported assignments:
    // (scalar=scalar, vector=vector, scalar=vector, vector=scalar etc)

    OperandTypes operandTypes = UNSUPPORTED;
    if (isCharType(lhsType, mContext) && isCharType(rhsType, mContext)) {
        operandTypes = STRING_OP_STRING;
    }
    else if (!(isCharType(lhsType, mContext) && isCharType(rhsType, mContext))) {
        const bool lhsIsArray = isArrayType(lhsType);
        const bool rhsIsArray = isArrayType(rhsType);
        if (lhsIsArray && rhsIsArray)        operandTypes = ARRAY_OP_ARRAY;
        else if (!lhsIsArray && rhsIsArray)  operandTypes = SCALAR_OP_ARRAY;
        else if (lhsIsArray && !rhsIsArray)  operandTypes = ARRAY_OP_SCALAR;
        else                                 operandTypes = SCALAR_OP_SCALAR;
    }

    const ast::tokens::OperatorToken op = node.mOperation;
    std::vector<llvm::Value*> lhsElements, rhsElements;

    // Initialise array elements

    switch (operandTypes) {
        case SCALAR_OP_ARRAY : {
            arrayUnpack(ptrToRhsValue, rhsElements, mBuilder, /*load elements*/true);
            llvm::Value* lhs = mBuilder.CreateLoad(ptrToLhsValue);

            llvm::Type* type = typePrecedence(lhsType, rhsType->getArrayElementType());
            lhs = arithmeticConversion(lhs, type, mBuilder);
            arithmeticConversion(rhsElements, type, mBuilder);

            lhsElements.resize(rhsElements.size());
            std::fill(lhsElements.begin(), lhsElements.end(), lhs);
            break;
        }
        case ARRAY_OP_SCALAR : {
            arrayUnpack(ptrToLhsValue, lhsElements, mBuilder, /*load elements*/true);
            llvm::Value* rhs = mBuilder.CreateLoad(ptrToRhsValue);

            llvm::Type* type = typePrecedence(lhsType->getArrayElementType(), rhsType);
            rhs = arithmeticConversion(rhs, type, mBuilder);
            arithmeticConversion(lhsElements, type, mBuilder);

            rhsElements.resize(lhsElements.size());
            std::fill(rhsElements.begin(), rhsElements.end(), rhs);
            break;
        }
        case ARRAY_OP_ARRAY : {
            const size_t lhsSize = lhsType->getArrayNumElements();
            const size_t rhsSize = rhsType->getArrayNumElements();
            assert(lhsSize != 0 && rhsSize != 0);

            if (lhsSize != rhsSize) {
                OPENVDB_THROW(LLVMBinaryOperationError, "Unable to perform operation \""
                    + ast::tokens::operatorNameFromToken(op) + "\" on arrays of mismtaching sizes.");
            }

            // unpack both vectors
            arrayUnpack(ptrToRhsValue, rhsElements, mBuilder, /*load elements*/true);
            arrayUnpack(ptrToLhsValue, lhsElements, mBuilder, /*load elements*/true);

            // implicit cast if necessary
            llvm::Type* type = typePrecedence(lhsType->getArrayElementType(), rhsType->getArrayElementType());
            arithmeticConversion(lhsElements, type, mBuilder);
            arithmeticConversion(rhsElements, type, mBuilder);
            break;
        }
        default : {}
    }

    // Perform binary operations

    llvm::Value* resultPtr = nullptr;

    switch (operandTypes) {
        case SCALAR_OP_ARRAY :
        case ARRAY_OP_SCALAR :
        case ARRAY_OP_ARRAY  : {

            assert(!lhsElements.empty());
            assert(lhsElements.size() == rhsElements.size());
            const size_t results = lhsElements.size();

            auto lhsIter = lhsElements.begin();
            auto rhsIter = rhsElements.begin();

            const ast::tokens::OperatorType opType = ast::tokens::operatorType(op);

            if (opType == ast::tokens::ARITHMETIC) {

                resultPtr = mBuilder.CreateAlloca(llvm::ArrayType::get(lhsElements.front()->getType(), results));
                std::vector<llvm::Value*> arrayElements;
                arrayUnpack(resultPtr, arrayElements, mBuilder, /*load elements*/false);

                auto arrayResults = arrayElements.begin();
                for (; lhsIter != lhsElements.end(); ++lhsIter, ++rhsIter, ++arrayResults) {
                    llvm::Value* elementResult = binaryOperator(*lhsIter, *rhsIter, op, mBuilder);
                    mBuilder.CreateStore(elementResult, *arrayResults);
                }
            }
            else if (opType == ast::tokens::RELATIONAL) {

                // All relational vector ops return a scalar. The follow ops only work on the first
                // vector element:
                //   ast::tokens::MORETHAN,
                //   ast::tokens::LESSTHAN,
                //   ast::tokens::MORETHANOREQUAL
                //   ast::tokens::LESSTHANOREQUAL

                llvm::Value* elementResult = binaryOperator(*lhsIter, *rhsIter, op, mBuilder);
                ++lhsIter; ++rhsIter;

                if (op == ast::tokens::EQUALSEQUALS) {
                    for (; lhsIter != lhsElements.end(); ++lhsIter, ++rhsIter) {
                        llvm::Value* nextElementResult = binaryOperator(*lhsIter, *rhsIter, op, mBuilder);
                        elementResult = binaryOperator(nextElementResult, elementResult, ast::tokens::AND, mBuilder);
                    }
                }
                else if (op == ast::tokens::NOTEQUALS) {
                    for (; lhsIter != lhsElements.end(); ++lhsIter, ++rhsIter) {
                        llvm::Value* nextElementResult = binaryOperator(*lhsIter, *rhsIter, op, mBuilder);
                        elementResult = binaryOperator(nextElementResult, elementResult, ast::tokens::OR, mBuilder);
                    }
                }
                else if (op != ast::tokens::MORETHAN &&
                         op != ast::tokens::LESSTHAN &&
                         op != ast::tokens::MORETHANOREQUAL &&
                         op != ast::tokens::LESSTHANOREQUAL) {
                    OPENVDB_THROW(LLVMTokenError, "BinaryOperator - Unsupported RELATIONAL operator token");
                }

                llvm::Type* returnType = elementResult->getType();
                resultPtr = mBuilder.CreateAlloca(returnType);
                mBuilder.CreateStore(elementResult, resultPtr);
            }
            else if (opType == ast::tokens::LOGICAL || opType == ast::tokens::BITWISE) {
                OPENVDB_THROW(LLVMBinaryOperationError, "Call to unsupported operator \""
                    + ast::tokens::operatorNameFromToken(op) + "\" with a vector argument");
            }
            else {
                OPENVDB_THROW(LLVMTokenError, "BinaryOperator - Unrecognised operator category");
            }
            break;
        }
        case STRING_OP_STRING : {
            if (!mTargetLibInfoImpl) {
                OPENVDB_THROW(LLVMTargetError, "Target library information has not been initialize. "
                    "Attempted to call strcpy().");
            }
            if (op != ast::tokens::PLUS) {
                OPENVDB_THROW(LLVMBinaryOperationError, "Unsupported string operation \""
                    + ast::tokens::operatorNameFromToken(op) + "\"");
            }

            llvm::AllocaInst* lhsAllocInstance = llvm::cast<llvm::AllocaInst>(ptrToLhsValue);
            llvm::AllocaInst* rhsAllocInstance = llvm::cast<llvm::AllocaInst>(ptrToRhsValue);
            llvm::Value* lhsCharArraySize = lhsAllocInstance->getArraySize();
            llvm::Value* rhsCharArraySize = rhsAllocInstance->getArraySize();

            assert(llvm::isa<llvm::ConstantInt>(lhsCharArraySize));
            assert(llvm::isa<llvm::ConstantInt>(rhsCharArraySize));

            // subtract null terminator from lhs size

            llvm::Value* one = LLVMType<int64_t>::get(mContext, 1);
            lhsCharArraySize = binaryOperator(lhsCharArraySize, one, ast::tokens::MINUS, mBuilder);

            // add the sizes of both strings. The rhs include a null terminator

            llvm::Value* totalSize =
                binaryOperator(lhsCharArraySize, rhsCharArraySize, ast::tokens::PLUS, mBuilder);
            resultPtr = mBuilder.CreateAlloca(lhsType, totalSize);

            llvm::Value* rhsStringStartPtr = mBuilder.CreateGEP(resultPtr, lhsCharArraySize);

            // simple wrapper around the tli impl.
            const llvm::TargetLibraryInfo info(*mTargetLibInfoImpl);

            // strcpy into new string

            bool success = llvm::emitStrNCpy(/*dst*/resultPtr, /*src*/ptrToLhsValue, lhsCharArraySize, mBuilder, &info);
            success |= static_cast<bool>
                (llvm::emitStrNCpy(/*dst*/rhsStringStartPtr,
                    /*src*/ptrToRhsValue, rhsCharArraySize, mBuilder, &info));

            if (!success) {
                OPENVDB_THROW(LLVMTargetError, "Attempted to call strcpy() which is unsupported on "
                    "this target.");
            }
            break;
        }
        case SCALAR_OP_SCALAR : {
            // load and implicit conversion
            llvm::Value* rhs = mBuilder.CreateLoad(ptrToRhsValue);
            llvm::Value* lhs = mBuilder.CreateLoad(ptrToLhsValue);

            // convert to highest type precision
            arithmeticConversion(rhs, lhs, mBuilder);
            llvm::Value* result = binaryOperator(lhs, rhs, op, mBuilder);

            // create a store for the result
            resultPtr = mBuilder.CreateAlloca(result->getType());
            mBuilder.CreateStore(result, resultPtr);
            break;
        }
        default : {
            OPENVDB_THROW(LLVMCastError, "Unsupported implicit cast in binary operation.");
        }
    }

    assert(resultPtr);
    mValues.push(resultPtr);
}

void ComputeGenerator::visit(const ast::UnaryOperator& node)
{
    // If the unary operation is a +, keep the value ptr on the stack and
    // continue (avoid any new allocations or unecessary loads)

    const ast::tokens::OperatorToken token = node.mOperation;
    if (token == ast::tokens::PLUS) return;

    llvm::Value* value = mValues.top(); mValues.pop();
    assert(value && value->getType()->isPointerTy() &&
        "Input value to UnaryOperator is not a pointer type.");

    llvm::Value* resultPtr = nullptr;
    llvm::Type* type = value->getType()->getContainedType(0);

    if (type->isIntegerTy()) {

        // load the value
        value = mBuilder.CreateLoad(value);
        llvm::Value* result = nullptr;

        if (token == ast::tokens::MINUS)        result = mBuilder.CreateNeg(value);
        else if (token == ast::tokens::BITNOT)  result = mBuilder.CreateNot(value);
        else if (token == ast::tokens::NOT) {
            if (type->isIntegerTy(1))           result = mBuilder.CreateICmpEQ(value, llvm::ConstantInt::get(type, 0));
            else                                result = mBuilder.CreateICmpEQ(value, llvm::ConstantInt::getSigned(type, 0));
        }
        else {
            OPENVDB_THROW(LLVMTokenError, "Unrecognised int operator \"" +
                ast::tokens::operatorNameFromToken(token) + "\"");
        }

        // create a store for the result
        resultPtr = mBuilder.CreateAlloca(type);
        mBuilder.CreateStore(result, resultPtr);

    }
    else if (type->isFloatingPointTy()) {

        // load the value
        value = mBuilder.CreateLoad(value);
        llvm::Value* result = nullptr;

        if (token == ast::tokens::MINUS)         result = mBuilder.CreateFNeg(value);
        else if (token == ast::tokens::NOT)      result = mBuilder.CreateFCmpOEQ(value, llvm::ConstantFP::get(type, 0));
        else if (token == ast::tokens::BITNOT) {
            OPENVDB_THROW(LLVMUnaryOperationError, "Unable to perform operation \""
                + ast::tokens::operatorNameFromToken(token) + "\" on floating points values");
        }
        else {
            OPENVDB_THROW(LLVMTokenError, "Unrecognised FP operator \"" +
                ast::tokens::operatorNameFromToken(token) + "\"");
        }

        // create a store for the result
        resultPtr = mBuilder.CreateAlloca(type);
        mBuilder.CreateStore(result, resultPtr);
    }
    else if (type->isArrayTy()) {

        std::vector<llvm::Value*> elements;
        arrayUnpack(value, elements, mBuilder, /*load elements*/true);

        type = elements.front()->getType();

        if (token == ast::tokens::MINUS) {
            if (type->isFloatingPointTy()) {
                for (llvm::Value*& value : elements) {
                    value = mBuilder.CreateFNeg(value);
                }
            }
            else if (type->isIntegerTy()) {
                for (llvm::Value*& value : elements) {
                    value = mBuilder.CreateNeg(value);
                }
            }
            else {
                OPENVDB_THROW(LLVMUnaryOperationError, "Unrecognised array element type");
            }
        }
        else if (token == ast::tokens::NOT || token == ast::tokens::BITNOT) {
            OPENVDB_THROW(LLVMUnaryOperationError, "Unable to perform operation \""
                + ast::tokens::operatorNameFromToken(token) + "\" on arrays/vectors");
        }
        else {
            OPENVDB_THROW(LLVMTokenError, "Unrecognised array operator \"" +
                ast::tokens::operatorNameFromToken(token) + "\"");
        }

        resultPtr = arrayPack(elements, mBuilder);
    }
    else {
        OPENVDB_THROW(LLVMUnaryOperationError, "Value is not a scalar or vector");
    }

    // push result on to the stack
    mValues.push(resultPtr);
}

void ComputeGenerator::visit(const ast::AssignExpression& node)
{
    // Enum of supported assignments within the ComputeGenerator

    enum AssignmentType
    {
        UNSUPPORTED = 0,
        STRING_EQ_STRING,
        ARRAY_EQ_ARRAY,
        SCALAR_EQ_ARRAY,
        ARRAY_EQ_SCALAR,
        SCALAR_EQ_SCALAR
    };

    // values are not loaded. rhs and lhs are pointers to values

    llvm::Value* ptrToLhsValue = mValues.top(); mValues.pop();
    llvm::Value* ptrToRhsValue = mValues.top(); mValues.pop();

    assert(ptrToRhsValue && ptrToRhsValue->getType()->isPointerTy() &&
        "Right Hand Size input to AssignExpression is not a pointer type.");
    assert(ptrToLhsValue && ptrToLhsValue->getType()->isPointerTy() &&
        "Left Hand Size input to AssignExpression is not a pointer type.");
    assert(node.mVariable);

    // Push the original RHS value back onto stack to allow for multiple
    // assignment statements to be chained together

    mValues.push(ptrToRhsValue);

    llvm::Type* lhsType = ptrToLhsValue->getType()->getContainedType(0);
    llvm::Type* rhsType = ptrToRhsValue->getType()->getContainedType(0);

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

    std::vector<llvm::Value*> lhsElements, rhsElements;

    switch (assignmentType) {
        case STRING_EQ_STRING : {
            if (!mTargetLibInfoImpl) {
                OPENVDB_THROW(LLVMTargetError, "Target library information has not been initialize. "
                    "Attempted to call strcpy().");
            }

            // re-allocate the lhs
            llvm::AllocaInst* allocInstance = llvm::cast<llvm::AllocaInst>(ptrToRhsValue);
            llvm::Value* size = allocInstance->getArraySize();
            assert(llvm::isa<llvm::ConstantInt>(size));
            ptrToLhsValue = mBuilder.CreateAlloca(lhsType, size);

            // simple wrapper around the tli impl.
            const llvm::TargetLibraryInfo info(*mTargetLibInfoImpl);

            // call C strcpy() from rhs to lhs
            // @todo add internally supported loop function for string copy

            if (!llvm::emitStrNCpy(/*dst*/ptrToLhsValue, /*src*/ptrToRhsValue, size, mBuilder, &info)) {
                OPENVDB_THROW(LLVMTargetError, "Attempted to call strcpy() which is unsupported on "
                    "this target.");
            }

            // find and update the value

            mSymbolTables.replace(node.mVariable->mName, ptrToLhsValue);
            break;
        }
        case ARRAY_EQ_ARRAY : {
            const size_t lhsSize = lhsType->getArrayNumElements();
            const size_t rhsSize = rhsType->getArrayNumElements();
            if (lhsSize != rhsSize) {
                OPENVDB_THROW(LLVMArrayError, "Unable to assign vector/array "
                    "attributes with mismatching sizes");
            }

            // unpack both vectors (load rhs) and convert rhs to the lhs scalar type
            arrayUnpack(ptrToRhsValue, rhsElements, mBuilder, /*load elements*/true);
            arrayUnpack(ptrToLhsValue, lhsElements, mBuilder, /*load elements*/false);

            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            for (llvm::Value*& value : rhsElements) {
                value = arithmeticConversion(value, lhsElementType, mBuilder);
            }

            break;
        }
        case SCALAR_EQ_ARRAY : {
            // take the first value of the rhs array
            llvm::Value* rhs = arrayIndexUnpack(ptrToRhsValue, 0, mBuilder);
            rhs = mBuilder.CreateLoad(rhs);
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);

            rhsElements.emplace_back(rhs);
            lhsElements.emplace_back(ptrToLhsValue);

            break;
        }
        case ARRAY_EQ_SCALAR : {
            // vector = scalar assignment, convert rhs to a vector
            llvm::Value* rhs = mBuilder.CreateLoad(ptrToRhsValue);
            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            rhs = arithmeticConversion(rhs, lhsElementType, mBuilder);

            rhsElements.resize(lhsType->getArrayNumElements(), rhs);
            arrayUnpack(ptrToLhsValue, lhsElements, mBuilder, /*load elements*/false);

            break;
        }
        case SCALAR_EQ_SCALAR : {
            // load and implicit conversion
            llvm::Value* rhs = mBuilder.CreateLoad(ptrToRhsValue);
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);

            rhsElements.emplace_back(rhs);
            lhsElements.emplace_back(ptrToLhsValue);

            break;
        }
        default : {
            OPENVDB_THROW(LLVMCastError, "Unsupported implicit cast in assignment.");
        }
    }

    assert(lhsElements.size() == rhsElements.size());

    // store the loaded rhs elements in the unloaded (pointers)
    // to lhs elements

    auto lhsIter = lhsElements.begin();
    auto rhsIter = rhsElements.begin();
    for (; lhsIter != lhsElements.end(); ++lhsIter, ++rhsIter) {
        mBuilder.CreateStore(*rhsIter, *lhsIter);
    }
}

void ComputeGenerator::visit(const ast::Crement& node)
{
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

    mBuilder.CreateStore(rhs, lhs);

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

void ComputeGenerator::visit(const ast::FunctionCall& node)
{
    assert(node.mArguments.get() && ("Uninitialized expression list for " +
        node.mFunction).c_str());

    const FunctionBase::Ptr function = this->getFunction(node.mFunction, mOptions);

    if (!(function->context() & FunctionBase::Base)) {
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

void ComputeGenerator::visit(const ast::Cast& node)
{
    llvm::Value* value = mValues.top();
    llvm::Type* type = llvmTypeFromName(node.mType, mContext);

    // If the value to cast is already the correct type, return
    if (type == value->getType()->getContainedType(0)) return;

    mValues.pop();

    value = mBuilder.CreateLoad(value);
    value = arithmeticConversion(value, type, mBuilder);

    llvm::Value* store = mBuilder.CreateAlloca(value->getType());
    mBuilder.CreateStore(value, store);

    mValues.push(store);
}

void ComputeGenerator::visit(const ast::Return& node)
{
    mBuilder.CreateRetVoid();
    mReturnBlocks.push_back(llvm::BasicBlock::Create(mContext, "return", mFunction));
    mBuilder.SetInsertPoint(mReturnBlocks.back());
}

void ComputeGenerator::visit(const ast::DeclareLocal& node)
{
    // create storage for the local value.
    llvm::Value* value = mBuilder.CreateAlloca(llvmTypeFromName(node.mType, mContext));
    mValues.push(value);

    SymbolTable* current = mSymbolTables.getOrInsert(mCurrentBlock);
    if (!current->insert(node.mName, value)) {
        OPENVDB_THROW(LLVMDeclarationError, "Local variable \"" + node.mName +
            "\" has already been declared!");
    }

    if (mWarnings) {
        if (mSymbolTables.find(node.mName, mCurrentBlock - 1)) {
            mWarnings->emplace_back("Declaration of variable \"" + node.mName
                + "\" shadows a previous declaration.");
        }
    }
}

void ComputeGenerator::visit(const ast::Local& node)
{
    // Reverse iterate through the current blocks and use the first declaration found
    // The current block number looks something like as follows
    //
    // ENTRY: Block 1
    //
    // if(...) // Block 3
    // {
    //     if(...) {} // Block 5
    // }
    // else {} // Block 2
    //
    // Note that in block 5, block 2 variables will be queried. However block variables
    // are constructed from the top down, so although the block number is used for
    // reverse iterating, block 2 will not contain any information
    //

    llvm::Value* value = mSymbolTables.find(node.mName);
    if (value) {
        mValues.push(value);
    }
    else {
        OPENVDB_THROW(LLVMDeclarationError, "Variable \"" + node.mName +
            "\" hasn't been declared!");
    }
}

void ComputeGenerator::visit(const ast::VectorUnpack& node)
{
    assert(node.mIndex >= 0 && node.mIndex < 3 &&
           "mIndex out of range for querying vector 3");

    llvm::Value* value = mValues.top(); mValues.pop();

    if (!isArray3Type(value->getType()->getContainedType(0))) {
        OPENVDB_THROW(LLVMArrayError, "Attribute or Local variable is not a Vector type!");
    }

    // use GEP2 as we index into a pointer (0) and then into the element of the array
    // it's pointing to (node.mIndex)

    llvm::Value* element = mBuilder.CreateConstGEP2_64(value, 0, node.mIndex);
    mValues.push(element);
}

void ComputeGenerator::visit(const ast::VectorPack& node)
{
    llvm::Value* value1 = mValues.top(); mValues.pop();
    llvm::Value* value2 = mValues.top(); mValues.pop();
    llvm::Value* value3 = mValues.top(); mValues.pop();

    value1 = mBuilder.CreateLoad(value1);
    value2 = mBuilder.CreateLoad(value2);
    value3 = mBuilder.CreateLoad(value3);

    llvm::Value* array = array3Pack(value1, value2, value3, mBuilder);
    mValues.push(array);
}

void ComputeGenerator::visit(const ast::ArrayPack& node)
{
    assert(node.mArguments && "No valid array arguments");

    const size_t num = node.mArguments->mList.size();

    // if there is only one element on the stack, leave it as a pointer to a scalar
    // or another array
    if (num == 1) return;

    std::vector<llvm::Value*> values;
    values.reserve(num);
    for (size_t i = 0; i < num; ++i) {
        values.push_back(mBuilder.CreateLoad(mValues.top()));
        mValues.pop();
    }

    llvm::Value* array = arrayPackCast(values, mBuilder);
    mValues.push(array);
}

void ComputeGenerator::visit(const ast::Value<bool>& node)
{
    llvm::Constant* value = LLVMType<bool>::get(mContext, node.mValue);
    llvm::Value* store = mBuilder.CreateAlloca(value->getType());
    mBuilder.CreateStore(value, store);
    mValues.push(store);
}

void ComputeGenerator::visit(const ast::Value<int16_t>& node)
{
    visit<int16_t>(node);
}

void ComputeGenerator::visit(const ast::Value<int32_t>& node)
{
    visit<int32_t>(node);
}

void ComputeGenerator::visit(const ast::Value<int64_t>& node)
{
    visit<int64_t>(node);
}

void ComputeGenerator::visit(const ast::Value<float>& node)
{
    visit<float>(node);
}

void ComputeGenerator::visit(const ast::Value<double>& node)
{
    visit<double>(node);
}

void ComputeGenerator::visit(const ast::Value<std::string>& node)
{
    /// @note  Strings are currently implemented as arrays of an unknown size. They are not
    /// llvm array types, but more simply blocks of allocated data of size uint8_t (see Types.h).
    /// Although this is sightly less convenient than arrays, this is done to support the case
    /// where it is not possible to know the string size at compile time (string attributes).
    /// In this case a strcpy is required and negates the optimization effects of llvm arrays.
    /// Thus all strings are allocated as such to provide consistency.

    llvm::Value* store = llvmStringToValue(node.mValue, mBuilder);
    mValues.push(store);
}

FunctionBase::Ptr ComputeGenerator::getFunction(const std::string &identifier,
                                                const FunctionOptions &op,
                                                const bool allowInternal)
{
    FunctionBase::Ptr function = mFunctionRegistry.getOrInsert(identifier, op, allowInternal);
    if (!function) {
        OPENVDB_THROW(LLVMFunctionError, "Unable to locate function \"" + identifier + "\".");
    }
    return function;
}

template <typename ValueType>
typename std::enable_if<std::is_integral<ValueType>::value>::type
ComputeGenerator::visit(const ast::Value<ValueType>& node)
{
    using ContainerT = typename ast::Value<ValueType>::ContainerType;

    const ValueType literal = static_cast<ValueType>(node.mValue);

    if (mWarnings) {
        static const ContainerT max = static_cast<ContainerT>(std::numeric_limits<ValueType>::max());
        if (node.mText) {
            mWarnings->emplace_back("Integer constant is too large to be represented: " + *(node.mText)
                + " will be converted to \"" + std::to_string(literal) + "\"");
        }
        else if (node.mValue > max) {
            mWarnings->emplace_back("Signed integer overflow: " + std::to_string(node.mValue)
                + " cannot be represented in type '" + openvdb::typeNameAsString<ValueType>() + "'");
        }
    }

    llvm::Constant* value = LLVMType<ValueType>::get(mContext, literal);
    llvm::Value* store = mBuilder.CreateAlloca(value->getType());
    mBuilder.CreateStore(value, store);
    mValues.push(store);
}

template <typename ValueType>
typename std::enable_if<std::is_floating_point<ValueType>::value>::type
ComputeGenerator::visit(const ast::Value<ValueType>& node)
{
    using ContainerT = typename ast::Value<ValueType>::ContainerType;

    assert(std::isinf(node.mValue) || node.mValue >= static_cast<ContainerT>(0.0));
    const ValueType literal = static_cast<ValueType>(node.mValue);

    if (mWarnings) {
        static const ContainerT max = static_cast<ContainerT>(std::numeric_limits<ValueType>::max());
        if (node.mText) {
            mWarnings->emplace_back("Floating constant exceeds range of double: " + *(node.mText)
                + ". Converted to inf.");
        }
        else if (node.mValue > max) {

            mWarnings->emplace_back("Floating point overflow: " + std::to_string(node.mValue)
                + " cannot be represented in type '" + openvdb::typeNameAsString<ValueType>()
                + "'. Converted to \"" + std::to_string(literal) + "\"");
        }
    }

    llvm::Constant* value = LLVMType<ValueType>::get(mContext, literal);
    llvm::Value* store = mBuilder.CreateAlloca(value->getType());
    mBuilder.CreateStore(value, store);
    mValues.push(store);
}

void ComputeGenerator::visit(const ast::ExternalVariable& node)
{
    const std::string globalName = getGlobalExternalAccess(node.mName, node.mType);
    llvm::Value* ptrToAddress = this->globals().get(globalName);

    if (!ptrToAddress) {
        ptrToAddress = llvm::cast<llvm::GlobalVariable>
            (mModule.getOrInsertGlobal(globalName, LLVMType<uintptr_t>::get(mContext)));
        this->globals().insert(globalName, ptrToAddress);
    }

    llvm::Type* type = llvmTypeFromName(node.mType, mContext);
    llvm::Value* address = mBuilder.CreateLoad(ptrToAddress);
    llvm::Value* value = mBuilder.CreateIntToPtr(address, type->getPointerTo(0));

    mValues.push(value);
}

void ComputeGenerator::visit(const ast::Tree& node)
{
    assert(mBlocks.size() == 1);
    mBuilder.CreateRetVoid();
    for (auto& block : mReturnBlocks) block->eraseFromParent();
}

void ComputeGenerator::visit(const ast::Attribute& node)
{
    OPENVDB_THROW(LLVMContextError,
        "Base ComputeGenerator attempted to generate code for an Attribute. "
        "PointComputeGenerator or VolumeComputeGenerator should be used for "
        "attribute accesses.");
}

void ComputeGenerator::visit(const ast::AttributeValue& node)
{
    OPENVDB_THROW(LLVMContextError,
        "Base ComputeGenerator attempted to generate code for an Attribute. "
        "PointComputeGenerator or VolumeComputeGenerator should be used for "
        "attribute accesses.");
}

}
}
}
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
