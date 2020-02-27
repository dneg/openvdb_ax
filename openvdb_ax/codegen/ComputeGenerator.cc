///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2020 DNEG
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
#include <llvm/Support/raw_os_ostream.h>
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
        { "custom_data" }
    };

    return arguments;
}

std::string ComputeKernel::getDefaultName() { return "ax.compute"; }


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


ComputeGenerator::ComputeGenerator(llvm::Module& module,
                                   const FunctionOptions& options,
                                   FunctionRegistry& functionRegistry,
                                   std::vector<std::string>* const warnings)
    : mModule(module)
    , mContext(module.getContext())
    , mBuilder(llvm::IRBuilder<>(module.getContext()))
    , mValues()
    , mBreakContinueStack()
    , mScopeIndex(1)
    , mSymbolTables()
    , mWarnings(warnings)
    , mFunction(nullptr)
    , mOptions(options)
    , mFunctionRegistry(functionRegistry) {}

bool ComputeGenerator::generate(const ast::Tree& tree)
{
    llvm::FunctionType* type =
        llvmFunctionTypeFromSignature<ComputeKernel::Signature>(mContext);

    mFunction = llvm::Function::Create(type,
        llvm::Function::ExternalLinkage,
        ComputeKernel::getDefaultName(),
        &mModule);

    // Set up arguments for initial entry

    llvm::Function::arg_iterator argIter = mFunction->arg_begin();
    const auto arguments = ComputeKernel::getArgumentKeys();
    auto keyIter = arguments.cbegin();

    for (; argIter != mFunction->arg_end(); ++argIter, ++keyIter) {
        argIter->setName(*keyIter);
    }

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(mContext,
        "entry_" + ComputeKernel::getDefaultName(), mFunction);
    mBuilder.SetInsertPoint(entry);

    return this->traverse(&tree);
}

bool ComputeGenerator::visit(const ast::Block* block)
{
    mScopeIndex++;

    // traverse the contents of the block
    const size_t children = block->children();

    for (size_t i = 0; i < children; ++i) {
        if (!this->traverse(block->child(i))) {
            return false;
        }
    }

    mSymbolTables.erase(mScopeIndex);
    mScopeIndex--;
    return true;
}

bool ComputeGenerator::visit(const ast::ConditionalStatement* cond)
{
    llvm::BasicBlock* postIfBlock = llvm::BasicBlock::Create(mContext, "block", mFunction);

    // generate conditional
    if (!this->traverse(cond->child(0))) return false;
    llvm::Value* condition = mValues.top(); mValues.pop();
    if (condition->getType()->isPointerTy()) {
        condition = mBuilder.CreateLoad(condition);
    }
    condition = boolComparison(condition, mBuilder);

    const bool hasElse = cond->hasElseBranch();
    llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(mContext, "then", mFunction);
    llvm::BasicBlock* elseBlock = hasElse ? llvm::BasicBlock::Create(mContext, "else", mFunction) : postIfBlock;

    mBuilder.CreateCondBr(condition, thenBlock, elseBlock);

    // generate if-then branch
    mBuilder.SetInsertPoint(thenBlock);
    if (!this->traverse(cond->thenBranch())) return false;
    mBuilder.CreateBr(postIfBlock);

    if (hasElse) {
        // generate else-then branch
        mBuilder.SetInsertPoint(elseBlock);
        if (!this->traverse(cond->elseBranch())) return false;
        mBuilder.CreateBr(postIfBlock);
    }

    // reset to continue block
    mBuilder.SetInsertPoint(postIfBlock);
    return true;
}

bool ComputeGenerator::visit(const ast::Loop* loop)
{
    mScopeIndex++;

    llvm::BasicBlock* postLoopBlock = llvm::BasicBlock::Create(mContext, "block", mFunction);
    llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create(mContext, "loop_condition", mFunction);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(mContext, "loop_body", mFunction);

    llvm::BasicBlock* postBodyBlock = conditionBlock;

    const ast::tokens::LoopToken loopType = loop->loopType();
    if (loopType == ast::tokens::LoopToken::FOR) {
        // init -> condition -> body -> iter -> condition ... continue

        // generate initial statement
        if (loop->hasInit()) {
            if (!this->traverse(loop->initial())) return false;
        }
        mBuilder.CreateBr(conditionBlock);

        // generate iteration
        if (loop->hasIter()) {
            llvm::BasicBlock* iterBlock = llvm::BasicBlock::Create(mContext, "loop_iteration", mFunction);
            postBodyBlock = iterBlock;

            mBuilder.SetInsertPoint(iterBlock);
            if (!this->traverse(loop->iteration())) return false;
            mBuilder.CreateBr(conditionBlock);
        }
    }
    else if (loopType == ast::tokens::LoopToken::DO)  {
        //  body -> condition -> body -> condition ... continue
        mBuilder.CreateBr(bodyBlock);
    }
    else if (loopType == ast::tokens::LoopToken::WHILE) {
        //  condition -> body -> condition ... continue
        mBuilder.CreateBr(conditionBlock);
    }
    else { // unrecognised loop type
        OPENVDB_THROW(LLVMCastError, "Unsupported loop type.");
    }

    // store the destinations for break and continue
    mBreakContinueStack.push({postLoopBlock, postBodyBlock});

    // generate loop body
    mBuilder.SetInsertPoint(bodyBlock);
    if (!this->traverse(loop->body())) return false;
    mBuilder.CreateBr(postBodyBlock);

    // generate condition
    mBuilder.SetInsertPoint(conditionBlock);
    if (!this->traverse(loop->condition())) return false;
    llvm::Value* condition = mValues.top(); mValues.pop();
    if (condition->getType()->isPointerTy()) {
        condition = mBuilder.CreateLoad(condition);
    }
    condition = boolComparison(condition, mBuilder);
    mBuilder.CreateCondBr(condition, bodyBlock, postLoopBlock);

    // reset to post loop block
    mBuilder.SetInsertPoint(postLoopBlock);

    // discard break and continue
    mBreakContinueStack.pop();

    // remove the symbol table created in this scope
    mSymbolTables.erase(mScopeIndex);
    mScopeIndex--;

    return true;
}

bool ComputeGenerator::visit(const ast::Keyword* node)
{
    const ast::tokens::KeywordToken keyw = node->keyword();
    if (keyw == ast::tokens::KeywordToken::RETURN) {
        mBuilder.CreateRetVoid();
    }
    else if (keyw == ast::tokens::KeywordToken::BREAK ||
             keyw == ast::tokens::KeywordToken::CONTINUE) {
        // find the parent loop, if it exists
        const ast::Node* child = node;
        const ast::Node* parentLoop = node->parent();
        while (parentLoop) {
            if (parentLoop->nodetype() == ast::Node::NodeType::LoopNode) {
                break;
            }
            child = parentLoop;
            parentLoop = child->parent();
        }
        if (!parentLoop) {
            OPENVDB_THROW(LLVMSyntaxError, "Keyword \"" +
                ast::tokens::keywordNameFromToken(keyw) + "\" used outside of loop.");
        }

        const std::pair<llvm::BasicBlock*, llvm::BasicBlock*>
            breakContinue = mBreakContinueStack.top();

        if (keyw == ast::tokens::KeywordToken::BREAK) {
            assert(breakContinue.first);
            mBuilder.CreateBr(breakContinue.first);
        }
        else if (keyw == ast::tokens::KeywordToken::CONTINUE) {
            assert(breakContinue.second);
            mBuilder.CreateBr(breakContinue.second);
        }
    }
    else {
        OPENVDB_THROW(LLVMKeywordError, "Unsupported keyword '\"" +
            ast::tokens::keywordNameFromToken(keyw) + "\"'" );
    }

    llvm::BasicBlock* nullBlock = llvm::BasicBlock::Create(mContext, "null", mFunction);
    // insert all remaining instructions in scope into a null block
    // this will incorporate all instructions that follow until new insert point is set
    mBuilder.SetInsertPoint(nullBlock);
    return true;
}

bool ComputeGenerator::visit(const ast::BinaryOperator* node)
{
    // Enum of supported operations

    enum OperandTypes
    {
        STRING_OP_STRING,
        ARRAY_OP_ARRAY,
        SCALAR_OP_ARRAY,
        ARRAY_OP_SCALAR,
        SCALAR_OP_SCALAR
    };

    llvm::Value* rhs = mValues.top(); mValues.pop();
    llvm::Value* lhs = mValues.top(); mValues.pop();

    llvm::Type* lhsType = lhs->getType();
    llvm::Type* rhsType = rhs->getType();

    if (lhsType->isPointerTy()) {
        lhsType = lhsType->getPointerElementType();
        if (lhsType->isIntegerTy() || lhsType->isFloatingPointTy()) {
            lhs = mBuilder.CreateLoad(lhs);
        }
    }
    if (rhsType->isPointerTy()) {
        rhsType = rhsType->getPointerElementType();
        if (rhsType->isIntegerTy() || rhsType->isFloatingPointTy()) {
            rhs = mBuilder.CreateLoad(rhs);
        }
    }

    // convert rhs to match lhs for all supported assignments:
    // (scalar=scalar, vector=vector, scalar=vector, vector=scalar etc)

    OperandTypes operandType;
    if (lhsType == LLVMType<AXString>::get(mContext) &&
        rhsType == LLVMType<AXString>::get(mContext)) {
        operandType = STRING_OP_STRING;
    }
    else if (lhsType->isArrayTy() && rhsType->isArrayTy()) {
        operandType = ARRAY_OP_ARRAY;
    }
    else if (lhsType->isArrayTy() &&
        (rhsType->isIntegerTy() ||
         rhsType->isFloatingPointTy())) {
        operandType = ARRAY_OP_SCALAR;
    }
    else if (rhsType->isArrayTy() &&
        (lhsType->isIntegerTy() ||
         lhsType->isFloatingPointTy())) {
        operandType = SCALAR_OP_ARRAY;
    }
    else if ((lhsType->isIntegerTy() || lhsType->isFloatingPointTy()) &&
             (rhsType->isIntegerTy() || rhsType->isFloatingPointTy())) {
        operandType = SCALAR_OP_SCALAR;
    }
    else {
        OPENVDB_THROW(LLVMCastError,
            "Unsupported implicit cast in binary operation.");
    }

    const ast::tokens::OperatorToken op = node->operation();
    const ast::tokens::OperatorType opType = ast::tokens::operatorType(op);

    llvm::Value* result = nullptr;

    switch (operandType) {
        case STRING_OP_STRING : {
            if (op != ast::tokens::PLUS) {
                OPENVDB_THROW(LLVMBinaryOperationError, "Unsupported string operation \""
                    + ast::tokens::operatorNameFromToken(op) + "\"");
            }

            llvm::Type* strType = LLVMType<AXString>::get(mContext);

            auto& B = mBuilder;
            auto structToString = [&B, strType](llvm::Value*& str) -> llvm::Value*
            {
                llvm::Value* size = nullptr;
                if (llvm::isa<llvm::Constant>(str)) {
                    llvm::Constant* zero =
                        llvm::cast<llvm::Constant>(LLVMType<int32_t>::get(B.getContext(), 0));
                    llvm::Constant* constant = llvm::cast<llvm::Constant>(str)->getAggregateElement(zero); // char*
                    str = constant;
                    constant = constant->stripPointerCasts();

                    // array size should include the null terminator
                    llvm::Type* arrayType = constant->getType()->getPointerElementType();
                    assert(arrayType->getArrayNumElements() > 0);

                    const size_t count = arrayType->getArrayNumElements() - 1;
                    assert(count < static_cast<size_t>(std::numeric_limits<AXString::SizeType>::max()));

                    size = LLVMType<AXString::SizeType>::get
                        (B.getContext(), static_cast<AXString::SizeType>(count));
                }
                else {
                    llvm::Value* rstrptr = B.CreateStructGEP(strType, str, 0); // char**
                    rstrptr = B.CreateLoad(rstrptr);
                    size = B.CreateStructGEP(strType, str, 1); // AXString::SizeType*
                    size = B.CreateLoad(size);
                    str = rstrptr;
                }

                return size;
            };

            // lhs and rhs get set to the char* arrays in structToString
            llvm::Value* lhsSize = structToString(lhs);
            llvm::Value* rhsSize = structToString(rhs);
            // rhs with null terminator
            llvm::Value* one = LLVMType<AXString::SizeType>::get(mContext, 1);
            llvm::Value* rhsTermSize = binaryOperator(rhsSize, one, ast::tokens::PLUS, mBuilder);
            // total and total with term
            llvm::Value* total = binaryOperator(lhsSize, rhsSize, ast::tokens::PLUS, mBuilder);
            llvm::Value* totalTerm = binaryOperator(lhsSize, rhsTermSize, ast::tokens::PLUS, mBuilder);

            // get ptrs to the new structs values
            result = insertStaticAlloca(mBuilder, strType);
            llvm::Value* string = mBuilder.CreateAlloca(LLVMType<char>::get(mContext), totalTerm);
            llvm::Value* strptr = mBuilder.CreateStructGEP(strType, result, 0); // char**
            llvm::Value* sizeptr = mBuilder.CreateStructGEP(strType, result, 1); // AXString::SizeType*

            // get rhs offset
            llvm::Value* stringRhsOffset = mBuilder.CreateGEP(string, lhsSize);

            // memcpy
#if LLVM_VERSION_MAJOR > 6
            mBuilder.CreateMemCpy(string, /*dest-align*/0, lhs, /*src-align*/0, lhsSize);
            mBuilder.CreateMemCpy(stringRhsOffset, /*dest-align*/0, rhs, /*src-align*/0, rhsTermSize);
#else
            mBuilder.CreateMemCpy(string, lhs, lhsSize, /*align*/0);
            mBuilder.CreateMemCpy(stringRhsOffset, rhs, rhsTermSize, /*align*/0);
#endif
            mBuilder.CreateStore(string, strptr);
            mBuilder.CreateStore(total, sizeptr);
            break;
        }
        case ARRAY_OP_SCALAR :
        case SCALAR_OP_ARRAY :
        case ARRAY_OP_ARRAY  : {
            std::vector<llvm::Value*> elements;

            if (operandType == SCALAR_OP_ARRAY) {
                // Scalar [+][-][*][/] etc Array (Array)
                // Scalar [==][!=][>][<] etc Array (Scalar Bool)

                // rhs is array
                const size_t count = rhsType->getArrayNumElements();
                elements.reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    llvm::Value* element = mBuilder.CreateConstGEP2_64(rhs, 0, i);
                    element = mBuilder.CreateLoad(element);
                    arithmeticConversion(lhs, element, mBuilder);
                    element = binaryOperator(lhs, element, op, mBuilder);
                    elements.emplace_back(element);
                }
            }
            else if (operandType == ARRAY_OP_SCALAR) {
                // Array [+][-][*][/] etc Scalar (Array)
                // Array [==][!=][>][<] etc Scalar (Scalar Bool)

                // lhs is array
                const size_t count = lhsType->getArrayNumElements();
                elements.reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    llvm::Value* element = mBuilder.CreateConstGEP2_64(lhs, 0, i);
                    element = mBuilder.CreateLoad(element);
                    arithmeticConversion(element, rhs, mBuilder);
                    element = binaryOperator(element, rhs, op, mBuilder);
                    elements.emplace_back(element);
                }
            }
            else {
                // Array [+][-][*][/] etc Array (Array)
                // Array [==][!=][>][<] etc Array (Scalar Bool)

                // both lhs and rhs are arrays
                const size_t lhsSize = lhsType->getArrayNumElements();
                const size_t rhsSize = rhsType->getArrayNumElements();
                assert(lhsSize != 0 && rhsSize != 0);

                if (op == ast::tokens::MULTIPLY) {
                    if (lhsSize > 4 && rhsSize > 4) {
                        // matrix matrix multiplication all handled through mmmult
                        const FunctionGroup::Ptr function = this->getFunction("mmmult", mOptions, /*internal*/true);
                        result = function->execute({lhs, rhs}, mBuilder);
                        assert(result);
                        mValues.push(result);
                        return true;
                    }
                    else if (lhsSize > 4 && rhsSize <= 4) {
                        // matrix vector multiplication all handled through pretransform
                        const FunctionGroup::Ptr function = this->getFunction("pretransform", mOptions);
                        result = function->execute({lhs, rhs}, mBuilder);
                        assert(result);
                        mValues.push(result);
                        return true;
                    }
                    else if (lhsSize <= 4 && rhsSize > 4) {
                        // vector matrix multiplication all handled through transform
                        const FunctionGroup::Ptr function = this->getFunction("transform", mOptions);
                        result = function->execute({lhs, rhs}, mBuilder);
                        assert(result);
                        mValues.push(result);
                        return true;
                    }
                }

                if (lhsSize != rhsSize) {
                    OPENVDB_THROW(LLVMBinaryOperationError, "Unable to perform operation \""
                        + ast::tokens::operatorNameFromToken(op) + "\" on arrays of mismatching sizes.");
                }

                elements.reserve(lhsSize);
                for (size_t i = 0; i < lhsSize; ++i) {
                    llvm::Value* relement = mBuilder.CreateConstGEP2_64(rhs, 0, i);
                    relement = mBuilder.CreateLoad(relement);
                    llvm::Value* lelement = mBuilder.CreateConstGEP2_64(lhs, 0, i);
                    lelement = mBuilder.CreateLoad(lelement);
                    arithmeticConversion(lelement, relement, mBuilder);
                    lelement = binaryOperator(lelement, relement, op, mBuilder);
                    elements.emplace_back(lelement);
                }
            }

            // Scalar == Array or Scalar != Array. Note that other comparisons
            // (<, > <=, >=) work on the first element of the array only

            if (op == ast::tokens::EQUALSEQUALS || op == ast::tokens::NOTEQUALS) {
                const ast::tokens::OperatorToken reductionOp =
                    op == ast::tokens::EQUALSEQUALS ?
                    ast::tokens::AND :
                    ast::tokens::OR;
                // == and != operators create bool types
                result = elements.front();
                assert(result->getType() == LLVMType<bool>::get(mContext));
                for (size_t i = 1; i < elements.size(); ++i) {
                    result = binaryOperator(result, elements[i], reductionOp, mBuilder);
                }
            }
            else if (op == ast::tokens::MORETHAN ||
                op == ast::tokens::LESSTHAN ||
                op == ast::tokens::MORETHANOREQUAL ||
                op == ast::tokens::LESSTHANOREQUAL) {
                // first element only
                result = elements.front();
                assert(result->getType() == LLVMType<bool>::get(mContext));
            }
            else if (opType == ast::tokens::LOGICAL ||
                     opType == ast::tokens::BITWISE) {
                OPENVDB_THROW(LLVMBinaryOperationError, "Call to unsupported operator \""
                    + ast::tokens::operatorNameFromToken(op) + "\" with a vector argument");
            }
            else {
                llvm::Type* arrayType =
                    llvm::ArrayType::get(elements.front()->getType(), elements.size());

                // Create the allocation at the start of the function block
                result = insertStaticAlloca(mBuilder, arrayType);

                for (size_t i = 0; i < elements.size(); ++i) {
                    llvm::Value* target = mBuilder.CreateConstGEP2_64(result, 0, i);
                    mBuilder.CreateStore(elements[i], target);
                }
            }
            break;
        }
        case SCALAR_OP_SCALAR : {
            // convert to highest type precision
            arithmeticConversion(rhs, lhs, mBuilder);
            result = binaryOperator(lhs, rhs, op, mBuilder);
            break;
        }
    }

    assert(result);
    mValues.push(result);
    return true;
}

bool ComputeGenerator::visit(const ast::UnaryOperator* node)
{
    // If the unary operation is a +, keep the value ptr on the stack and
    // continue (avoid any new allocations or unecessary loads)

    const ast::tokens::OperatorToken token = node->operation();
    if (token == ast::tokens::PLUS) return true;

    llvm::Value* value = mValues.top(); mValues.pop();
    llvm::Type* type = value->getType();
    if (type->isPointerTy()) {
        type = type->getPointerElementType();
        if (type->isIntegerTy() || type->isFloatingPointTy()) {
            value = mBuilder.CreateLoad(value);
        }
    }

    llvm::Value* result = nullptr;
    if (type->isIntegerTy()) {
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
    }
    else if (type->isFloatingPointTy()) {
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
    }
    else if (type->isArrayTy()) {
        const size_t elements = type->getArrayNumElements();
        result = insertStaticAlloca(mBuilder, type);
        type = type->getArrayElementType();

        if (token == ast::tokens::MINUS) {
            if (type->isFloatingPointTy()) {
                for (size_t i = 0; i < elements; ++i) {
                    llvm::Value* source = mBuilder.CreateConstGEP2_64(value, 0, i);
                    llvm::Value* target = mBuilder.CreateConstGEP2_64(result, 0, i);
                    source = mBuilder.CreateLoad(source);
                    source = mBuilder.CreateFNeg(source);
                    mBuilder.CreateStore(source, target);
                }
            }
            else if (type->isIntegerTy()) {
                for (size_t i = 0; i < elements; ++i) {
                    llvm::Value* source = mBuilder.CreateConstGEP2_64(value, 0, i);
                    llvm::Value* target = mBuilder.CreateConstGEP2_64(result, 0, i);
                    source = mBuilder.CreateLoad(source);
                    source = mBuilder.CreateNeg(source);
                    mBuilder.CreateStore(source, target);
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
    }
    else {
        OPENVDB_THROW(LLVMUnaryOperationError, "Value is not a scalar or vector");
    }

    assert(result);
    mValues.push(result);
    return true;
}

bool ComputeGenerator::visit(const ast::AssignExpression*)
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

    llvm::Value* lhs = mValues.top(); mValues.pop();
    if (!lhs->getType()->isPointerTy()) {
        OPENVDB_THROW(LLVMBinaryOperationError, "Unable to assign to an rvalue");
    }

    // Keep original RHS value back onto stack to allow for multiple
    // assignment statements to be chained together, but make sure its
    // only loaded once

    llvm::Value* rhs = mValues.top();
    llvm::Type* rhsType = rhs->getType();

    if (rhsType->isPointerTy()) {
        rhsType = rhsType->getPointerElementType();
        if (rhsType->isIntegerTy() || rhsType->isFloatingPointTy()) {
            mValues.pop();
            rhs = mBuilder.CreateLoad(rhs);
            mValues.push(rhs);
        }
    }

    llvm::Type* lhsType = lhs->getType()->getPointerElementType();

    // convert rhs to match lhs for all supported assignments:
    // (scalar=scalar, vector=vector, scalar=vector, vector=scalar etc)

    AssignmentType assignmentType = UNSUPPORTED;
    if (lhsType == LLVMType<AXString>::get(mContext) &&
        rhsType == LLVMType<AXString>::get(mContext)) {
        assignmentType = STRING_EQ_STRING;
    }
    else if (lhsType->isArrayTy() && rhsType->isArrayTy()) {
        assignmentType = ARRAY_EQ_ARRAY;
    }
    else if (lhsType->isArrayTy() &&
        (rhsType->isIntegerTy() ||
         rhsType->isFloatingPointTy())) {
        assignmentType = ARRAY_EQ_SCALAR;
    }
    else if (rhsType->isArrayTy() &&
        (lhsType->isIntegerTy() ||
         lhsType->isFloatingPointTy())) {
        assignmentType = SCALAR_EQ_ARRAY;
    }
    else if ((lhsType->isIntegerTy() || lhsType->isFloatingPointTy()) &&
             (rhsType->isIntegerTy() || rhsType->isFloatingPointTy())) {
        assignmentType = SCALAR_EQ_SCALAR;
    }

    switch (assignmentType) {
        case STRING_EQ_STRING : {

            // get the size of the rhs string
            llvm::Type* strType = LLVMType<AXString>::get(mContext);
            llvm::Value* rstrptr = nullptr;
            llvm::Value* size = nullptr;

            if (llvm::isa<llvm::Constant>(rhs)) {
                llvm::Constant* zero =
                    llvm::cast<llvm::Constant>(LLVMType<int32_t>::get(mContext, 0));
                llvm::Constant* constant = llvm::cast<llvm::Constant>(rhs)->getAggregateElement(zero); // char*
                rstrptr = constant;
                constant = constant->stripPointerCasts();
                const size_t count = constant->getType()->getPointerElementType()->getArrayNumElements();
                assert(count < static_cast<size_t>(std::numeric_limits<AXString::SizeType>::max()));

                size = LLVMType<AXString::SizeType>::get
                    (mContext, static_cast<AXString::SizeType>(count));
            }
            else {
                rstrptr = mBuilder.CreateStructGEP(strType, rhs, 0); // char**
                rstrptr = mBuilder.CreateLoad(rstrptr);
                size = mBuilder.CreateStructGEP(strType, rhs, 1); // AXString::SizeType*
                size = mBuilder.CreateLoad(size);
            }

            // total with term
            llvm::Value* one = LLVMType<AXString::SizeType>::get(mContext, 1);
            llvm::Value* totalTerm = binaryOperator(size, one, ast::tokens::PLUS, mBuilder);

            // re-allocate the string array
            llvm::Value* string = mBuilder.CreateAlloca(LLVMType<char>::get(mContext), totalTerm);
            llvm::Value* lstrptr = mBuilder.CreateStructGEP(strType, lhs, 0); // char**
            llvm::Value* lsize = mBuilder.CreateStructGEP(strType, lhs, 1); // AXString::SizeType*

#if LLVM_VERSION_MAJOR > 6
            mBuilder.CreateMemCpy(string, /*dest-align*/0, rstrptr, /*src-align*/0, totalTerm);
#else
            mBuilder.CreateMemCpy(string, rstrptr, totalTerm, /*align*/0);
#endif
            mBuilder.CreateStore(string, lstrptr);
            mBuilder.CreateStore(size, lsize);
            break;
        }
        case ARRAY_EQ_ARRAY : {
            // @note  This is technically possible, but we disallow it as a feature
            // of the language (currently)
            const size_t lhsSize = lhsType->getArrayNumElements();
            const size_t rhsSize = rhsType->getArrayNumElements();
            if (lhsSize != rhsSize) {
                OPENVDB_THROW(LLVMArrayError, "Unable to assign vector/array "
                    "attributes with mismatching sizes");
            }

            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            for (size_t i = 0; i < lhsSize; ++i) {
                llvm::Value* lelement = mBuilder.CreateConstGEP2_64(lhs, 0, i);
                llvm::Value* relement = mBuilder.CreateConstGEP2_64(rhs, 0, i);
                relement = mBuilder.CreateLoad(relement);
                relement = arithmeticConversion(relement, lhsElementType, mBuilder);
                mBuilder.CreateStore(relement, lelement);
            }
            break;
        }
        case SCALAR_EQ_ARRAY : {
            // take the first value of the rhs array
            rhs = mBuilder.CreateConstGEP2_64(rhs, 0, 0);
            rhs = mBuilder.CreateLoad(rhs);
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);
            mBuilder.CreateStore(rhs, lhs);
            break;
        }
        case ARRAY_EQ_SCALAR : {
            // vector = scalar assignment - implicit cast and element store
            llvm::Type* lhsElementType = lhsType->getArrayElementType();
            rhs = arithmeticConversion(rhs, lhsElementType, mBuilder);

            const size_t elements = lhsType->getArrayNumElements();
            for (size_t i = 0; i < elements; ++i) {
                llvm::Value* value = mBuilder.CreateConstGEP2_64(lhs, 0, i);
                mBuilder.CreateStore(rhs, value);
            }
            break;
        }
        case SCALAR_EQ_SCALAR : {
            // implicit conversion - scalars are already loaded
            rhs = arithmeticConversion(rhs, lhsType, mBuilder);
            mBuilder.CreateStore(rhs, lhs);
            break;
        }
        default : {
            OPENVDB_THROW(LLVMCastError, "Unsupported implicit cast in assignment.");
        }
    }
    return true;
}

bool ComputeGenerator::visit(const ast::Crement* node)
{
    llvm::Value* value = mValues.top(); mValues.pop();

    if (!value->getType()->isPointerTy()) {
        OPENVDB_THROW(LLVMBinaryOperationError, "Unable to assign to an rvalue");
    }

    llvm::Value* rvalue = mBuilder.CreateLoad(value);
    llvm::Type* type = rvalue->getType();

    if (!type->isIntegerTy() && !type->isFloatingPointTy()) {
        OPENVDB_THROW(LLVMTypeError, "Variable is an unsupported type for "
            "crement. Must be scalar.");
    }

    llvm::Value* crement = nullptr;
    if (node->increment())      crement = LLVMType<int32_t>::get(mContext, 1);
    else if (node->decrement()) crement = LLVMType<int32_t>::get(mContext, -1);
    else OPENVDB_THROW(LLVMTokenError, "Unrecognised crement operation token");

    crement = arithmeticConversion(crement, type, mBuilder);
    if (type->isIntegerTy())       crement = mBuilder.CreateAdd(rvalue, crement);
    if (type->isFloatingPointTy()) crement = mBuilder.CreateFAdd(rvalue, crement);

    mBuilder.CreateStore(crement, value);

    // decide what to put on the expression stack

    if (node->post()) mValues.push(rvalue);
    else             mValues.push(crement);
    return true;
}

bool ComputeGenerator::visit(const ast::FunctionCall* node)
{
    const FunctionGroup::Ptr function = this->getFunction(node->name(), mOptions);
    const size_t args = node->numArgs();
    assert(mValues.size() >= args);

    // initialize arguments. scalars are always passed by value, arrays
    // and strings always by pointer
    llvm::Type* strType = LLVMType<AXString>::get(mContext);

    std::vector<llvm::Value*> arguments;
    arguments.resize(args);

    for (auto r = arguments.rbegin(); r != arguments.rend(); ++r) {
        llvm::Value* arg = mValues.top(); mValues.pop();
        llvm::Type* type = arg->getType();
        if (type->isPointerTy()) {
            type = type->getPointerElementType();
            if (type->isIntegerTy() || type->isFloatingPointTy()) {
                // pass by value
                arg = mBuilder.CreateLoad(arg);
            }
            else if (type->isArrayTy()) {/*pass by pointer*/}
            else if (type == strType) {/*pass by pointer*/}
        }
        else {
            // arrays should never be loaded
            assert(!type->isArrayTy());
            assert(type != strType);
            if (type->isIntegerTy() || type->isFloatingPointTy()) {
                /*pass by value*/
            }
        }
        *r = arg;
    }

    llvm::Value* result = function->execute(arguments, mBuilder);
    assert(result);
    mValues.push(result);
    return true;
}

bool ComputeGenerator::visit(const ast::Cast* node)
{
    llvm::Value* value = mValues.top();
    llvm::Type* type =
        value->getType()->isPointerTy() ?
        value->getType()->getPointerElementType() :
        value->getType();

    if (!type->isIntegerTy() && !type->isFloatingPointTy()) {
        OPENVDB_THROW(LLVMTypeError, "Unable to cast non scalar values");
    }

    // If the value to cast is already the correct type, return
    llvm::Type* targetType = llvmTypeFromToken(node->type(), mContext);
    if (type == targetType) return true;

    mValues.pop();

    if (value->getType()->isPointerTy()) {
        value = mBuilder.CreateLoad(value);
    }

    value = arithmeticConversion(value, targetType, mBuilder);
    mValues.push(value);
    return true;
}

bool ComputeGenerator::visit(const ast::DeclareLocal* node)
{
    // create storage for the local value.
    llvm::Type* type = llvmTypeFromToken(node->type(), mContext);
    llvm::Value* value = insertStaticAlloca(mBuilder, type);

    // for strings, make sure we correctly initialize to the empty string.
    // strings are the only variable type that are currently default allocated
    // otherwise you can run into issues with binary operands
    if (node->type() == ast::tokens::STRING) {
        llvm::Value* loc = mBuilder.CreateGlobalStringPtr(""); // char*
        llvm::Constant* constLoc = llvm::cast<llvm::Constant>(loc);
        llvm::Constant* size = LLVMType<AXString::SizeType>::get
            (mContext, static_cast<AXString::SizeType>(0));
        llvm::Value* constStr = LLVMType<AXString>::get(mContext, constLoc, size);
        mBuilder.CreateStore(constStr, value);
    }

    mValues.push(value);
    SymbolTable* current = mSymbolTables.getOrInsert(mScopeIndex);
    if (!current->insert(node->name(), value)) {
        OPENVDB_THROW(LLVMDeclarationError, "Local variable \"" + node->name() +
            "\" has already been declared!");
    }

    if (mWarnings) {
        if (mSymbolTables.find(node->name(), mScopeIndex - 1)) {
            mWarnings->emplace_back("Declaration of variable \"" + node->name()
                + "\" shadows a previous declaration.");
        }
    }
    return true;
}

bool ComputeGenerator::visit(const ast::Local* node)
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

    llvm::Value* value = mSymbolTables.find(node->name());
    if (value) {
        mValues.push(value);
    }
    else {
        OPENVDB_THROW(LLVMDeclarationError, "Variable \"" + node->name() +
            "\" hasn't been declared!");
    }
    return true;
}

bool ComputeGenerator::visit(const ast::ArrayUnpack* node)
{
    llvm::Value* value = mValues.top(); mValues.pop();
    llvm::Value* component0 = mValues.top(); mValues.pop();
    llvm::Value* component1 = nullptr;

    if (node->isMatrixIndex()) {
        component1 = mValues.top(); mValues.pop();
        // if double indexing, the two component values will be
        // pushed onto the stack with the first index last. i.e.
        //      top: expression
        //           2nd index (if matrix access)
        //   bottom: 1st index
        // so swap the components
        std::swap(component0, component1);
    }

    llvm::Type* type = value->getType();
    if (!type->isPointerTy() ||
        !type->getPointerElementType()->isArrayTy()) {
        OPENVDB_THROW(LLVMArrayError,
            "Variable is not a valid type for component access.");
    }

    // type now guaranteed to be an array type
    type = type->getPointerElementType();
    const size_t size = type->getArrayNumElements();
    if (component1 && size <= 4) {
        OPENVDB_THROW(LLVMArrayError,
            "Attribute or Local variable is not a compatible matrix type "
            "for [,] indexing.");
    }

    if (component0->getType()->isPointerTy()) {
        component0 = mBuilder.CreateLoad(component0);
    }
    if (component1 && component1->getType()->isPointerTy()) {
        component1 = mBuilder.CreateLoad(component1);
    }

    if (!component0->getType()->isIntegerTy() ||
        (component1 && !component1->getType()->isIntegerTy())) {
        std::ostringstream os;
        llvm::raw_os_ostream stream(os);
        component0->getType()->print(stream);
        stream << ", ";
        component1->getType()->print(stream);
        stream.flush();
        OPENVDB_THROW(LLVMArrayError,
            "Unable to index into array with a non integer value. Types are ["
            + os.str() + "]");
    }

    llvm::Value* zero = LLVMType<int32_t>::get(mContext, 0);
    if (!component1) {
        value = mBuilder.CreateGEP(value, {zero, component0});
    }
    else {
        // component0 = row, component1 = column. Index into the matrix array
        // which is layed out in row major = (component0*dim + component1)
        assert(size == 9 || size == 16);
        const int32_t dim = size == 9 ? 3 : 4;
        llvm::Value* offset =
            LLVMType<int32_t>::get(mContext, static_cast<int32_t>(dim));
        component0 = binaryOperator(component0, offset, ast::tokens::MULTIPLY, mBuilder);
        component0 = binaryOperator(component0, component1, ast::tokens::PLUS, mBuilder);
        value = mBuilder.CreateGEP(value, {zero, component0});
    }

    mValues.push(value);
    return true;
}

bool ComputeGenerator::visit(const ast::ArrayPack* node)
{
    const size_t num = node->numArgs();

    // if there is only one element on the stack, leave it as a pointer to a scalar
    // or another array
    if (num == 1) return true;

    std::vector<llvm::Value*> values;
    values.reserve(num);
    for (size_t i = 0; i < num; ++i) {
        llvm::Value* value = mValues.top(); mValues.pop();
        if (value->getType()->isPointerTy()) {
            value = mBuilder.CreateLoad(value);
        }
        values.push_back(value);
    }

    // reserve the values
    // @todo this should probably be handled by the AST
    std::reverse(values.begin(), values.end());

    llvm::Value* array = arrayPackCast(values, mBuilder);
    mValues.push(array);
    return true;
}

bool ComputeGenerator::visit(const ast::Value<bool>* node)
{
    llvm::Constant* value = LLVMType<bool>::get(mContext, node->value());
    mValues.push(value);
    return true;
}

bool ComputeGenerator::visit(const ast::Value<int16_t>* node)
{
    return visit<int16_t>(node);
}

bool ComputeGenerator::visit(const ast::Value<int32_t>* node)
{
    return visit<int32_t>(node);
}

bool ComputeGenerator::visit(const ast::Value<int64_t>* node)
{
    return visit<int64_t>(node);
}

bool ComputeGenerator::visit(const ast::Value<float>* node)
{
    return visit<float>(node);
}

bool ComputeGenerator::visit(const ast::Value<double>* node)
{
    return visit<double>(node);
}

bool ComputeGenerator::visit(const ast::Value<std::string>* node)
{
    assert(node->value().size() <
        static_cast<size_t>(std::numeric_limits<AXString::SizeType>::max()));

    llvm::Value* loc = mBuilder.CreateGlobalStringPtr(node->value()); // char*
    llvm::Constant* constLoc = llvm::cast<llvm::Constant>(loc);

    llvm::Constant* size = LLVMType<AXString::SizeType>::get
        (mContext, static_cast<AXString::SizeType>(node->value().size()));
    llvm::Value* constStr = LLVMType<AXString>::get(mContext, constLoc, size);

    // Always allocate an AXString here for easier passing to functions
    // @todo shouldn't need an AXString for char* literals
    llvm::Value* alloc = insertStaticAlloca(mBuilder, LLVMType<AXString>::get(mContext));
    mBuilder.CreateStore(constStr, alloc);
    mValues.push(alloc);
    return true;
}

FunctionGroup::Ptr ComputeGenerator::getFunction(const std::string &identifier,
                                                const FunctionOptions &op,
                                                const bool allowInternal)
{
    FunctionGroup::Ptr function = mFunctionRegistry.getOrInsert(identifier, op, allowInternal);
    if (!function) {
        OPENVDB_THROW(LLVMFunctionError,
            "Unable to locate function \"" + identifier + "\".");
    }
    return function;
}

template <typename ValueType>
typename std::enable_if<std::is_integral<ValueType>::value, bool>::type
ComputeGenerator::visit(const ast::Value<ValueType>* node)
{
    using ContainerT = typename ast::Value<ValueType>::ContainerType;

    const ValueType literal = static_cast<ValueType>(node->value());

    if (mWarnings) {
        static const ContainerT max = static_cast<ContainerT>(std::numeric_limits<ValueType>::max());
        if (node->text()) {
            mWarnings->emplace_back("Integer constant is too large to be represented: " + *(node->text())
                + " will be converted to \"" + std::to_string(literal) + "\"");
        }
        else if (node->asContainerType() > max) {
            mWarnings->emplace_back("Signed integer overflow: " + std::to_string(node->asContainerType())
                + " cannot be represented in type '" + openvdb::typeNameAsString<ValueType>() + "'");
        }
    }

    llvm::Constant* value = LLVMType<ValueType>::get(mContext, literal);
    mValues.push(value);
    return true;
}

template <typename ValueType>
typename std::enable_if<std::is_floating_point<ValueType>::value, bool>::type
ComputeGenerator::visit(const ast::Value<ValueType>* node)
{
    using ContainerT = typename ast::Value<ValueType>::ContainerType;

    assert(std::isinf(node->value()) || node->value() >= static_cast<ContainerT>(0.0));
    const ValueType literal = static_cast<ValueType>(node->value());

    if (mWarnings) {
        static const ContainerT max = static_cast<ContainerT>(std::numeric_limits<ValueType>::max());
        if (node->text()) {
            mWarnings->emplace_back("Floating constant exceeds range of double: " + *(node->text())
                + ". Converted to inf.");
        }
        else if (node->asContainerType() > max) {
            mWarnings->emplace_back("Floating point overflow: " + std::to_string(node->asContainerType())
                + " cannot be represented in type '" + openvdb::typeNameAsString<ValueType>()
                + "'. Converted to \"" + std::to_string(literal) + "\"");
        }
    }

    llvm::Constant* value = LLVMType<ValueType>::get(mContext, literal);
    mValues.push(value);
    return true;
}

bool ComputeGenerator::visit(const ast::ExternalVariable* node)
{
    const std::string globalName = node->tokenname();
    llvm::Value* ptrToAddress = this->globals().get(globalName);

    if (!ptrToAddress) {
        ptrToAddress = llvm::cast<llvm::GlobalVariable>
            (mModule.getOrInsertGlobal(globalName, LLVMType<uintptr_t>::get(mContext)));
        this->globals().insert(globalName, ptrToAddress);
    }

    llvm::Type* type = llvmTypeFromToken(node->type(), mContext);
    llvm::Value* address = mBuilder.CreateLoad(ptrToAddress);
    llvm::Value* value = mBuilder.CreateIntToPtr(address, type->getPointerTo(0));
    if (type->isIntegerTy() || type->isFloatingPointTy()) {
        value = mBuilder.CreateLoad(value);
    }
    mValues.push(value);
    return true;
}

bool ComputeGenerator::visit(const ast::Tree*)
{
    // In case we haven't returned already (i.e. we are NOT in a null block)
    // we insert a ret void. If we are, this will just get cleaned up anyway below.
    mBuilder.CreateRetVoid();
    mBuilder.SetInsertPoint(&mFunction->back());
    return true;
}

bool ComputeGenerator::visit(const ast::Attribute*)
{
    OPENVDB_THROW(LLVMContextError,
        "Base ComputeGenerator attempted to generate code for an Attribute. "
        "PointComputeGenerator or VolumeComputeGenerator should be used for "
        "attribute accesses.");
    return false;
}

}
}
}
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
