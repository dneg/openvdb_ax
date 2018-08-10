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

#include "AST.h"

#include <openvdb_ax/Exceptions.h>

#include <tbb/mutex.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

Node::~Node()
{
}

Visitor::~Visitor()
{
}

void Tree::accept(Visitor& visitor) const
{
    visitor.init(*this);
    if (mBlock) mBlock->accept(visitor);
    visitor.visit(*this);
}

void Block::accept(Visitor& visitor) const
{
    for (const Statement::Ptr& statement : mList) {
        statement->accept(visitor);
    }

    visitor.visit(*this);
}

void ExpressionList::accept(Visitor& visitor) const
{
    for (const Expression::Ptr& expression : mList) {
        expression->accept(visitor);
    }

    visitor.visit(*this);
}

/*
 * For conditional statements:
 *  1) visit the condition and push the result to the value stack
 *  2) visit the conditional node and setup the block branches
 *  3) visit the first branch (if->then), visit the statements and setup end branches
 *  4) visit the second branch (else), visit the statements and setup end branches.
 *     Else branch will always exist, but will contain no statements if not specified
*/
void ConditionalStatement::accept(Visitor& visitor) const
{
    mConditional->accept(visitor);
    visitor.visit(*this);
    mThenBranch->accept(visitor);
    mElseBranch->accept(visitor);
}

void AssignExpression::accept(Visitor& visitor) const
{
    mExpression->accept(visitor);
    mVariable->accept(visitor);
    visitor.visit(*this);
}

void Crement::accept(Visitor& visitor) const
{
    mVariable->accept(visitor);
    mExpression->accept(visitor);
    visitor.visit(*this);
}

void UnaryOperator::accept(Visitor& visitor) const
{
    mExpression->accept(visitor);
    visitor.visit(*this);
}

void BinaryOperator::accept(Visitor& visitor) const
{
    mLeft->accept(visitor);
    mRight->accept(visitor);
    visitor.visit(*this);
}

void Cast::accept(Visitor& visitor) const
{
    mExpression->accept(visitor);
    visitor.visit(*this);
}

void FunctionCall::accept(Visitor& visitor) const
{
    mArguments->accept(visitor);
    visitor.visit(*this);
}

void Return::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

void Attribute::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

void AttributeValue::accept(Visitor& visitor) const
{
    mAttribute->accept(visitor);
    visitor.visit(*this);
}

void DeclareLocal::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

void Local::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

void LocalValue::accept(Visitor& visitor) const
{
    mLocal->accept(visitor);
    visitor.visit(*this);
}

void VectorUnpack::accept(Visitor& visitor) const
{
    mExpression->accept(visitor);
    visitor.visit(*this);
}

void VectorPack::accept(Visitor& visitor) const
{
    // Reverse order for easier stack interaction
    mValue3->accept(visitor);
    mValue2->accept(visitor);
    mValue1->accept(visitor);
    visitor.visit(*this);
}

void ArrayPack::accept(Visitor& visitor) const
{
    mArguments->accept(visitor);
    visitor.visit(*this);
}

template <typename T>
void Value<T>::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

void Value<std::string>::accept(Visitor& visitor) const
{
    visitor.visit(*this);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Modifier::~Modifier()
{
}

Tree* Tree::accept(Modifier& visitor)
{
    Block* node = nullptr;
    if (mBlock) node = mBlock->accept(visitor);
    if (node) {
        mBlock.reset(node);
    }

    return this;
}

Block* Block::accept(Modifier& visitor)
{
    if (Block* node = visitor.visit(*this)) {
        return node;
    }

    for (Statement::Ptr& statement : mList) {
        if (Node* node = statement->accept(visitor)) {
            statement.reset(static_cast<Statement*>(node));
        }
    }

    return nullptr;
}

ExpressionList* ExpressionList::accept(Modifier& visitor)
{
    if (ExpressionList* node = visitor.visit(*this)) {
        return node;
    }

    for (Expression::Ptr& expression : mList) {
        if (Node* node = expression->accept(visitor)) {
            expression.reset(static_cast<Expression*>(node));
        }
    }

    return nullptr;
}

Statement* ConditionalStatement::accept(Modifier& visitor)
{
    if (Statement* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mConditional->accept(visitor)) {
        mConditional.reset(static_cast<Expression*>(node));
    }
    if (Node* node = mElseBranch->accept(visitor)) {
        mElseBranch.reset(static_cast<Block*>(node));
    }
    if (Node* node = mThenBranch->accept(visitor)) {
        mThenBranch.reset(static_cast<Block*>(node));
    }

    return nullptr;
}

Expression* AssignExpression::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mVariable->accept(visitor)) {
        mVariable.reset(static_cast<Variable*>(node));
    }
    if (Node* node = mExpression->accept(visitor)) {
        mExpression.reset(static_cast<Expression*>(node));
    }

    return nullptr;
}

Expression* Crement::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mVariable->accept(visitor)) {
        mVariable.reset(static_cast<Variable*>(node));
    }
    if (Node* node = mExpression->accept(visitor)) {
        mExpression.reset(static_cast<Expression*>(node));
    }

    return nullptr;
}

Expression* UnaryOperator::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    if (Node* node = mExpression->accept(visitor)) {
        mExpression.reset(static_cast<Expression*>(node));
    }
    return nullptr;
}

Expression* BinaryOperator::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mLeft->accept(visitor)) {
        mLeft.reset(static_cast<Expression*>(node));
    }
    if (Node* node = mRight->accept(visitor)) {
        mRight.reset(static_cast<Expression*>(node));
    }
    return nullptr;
}

Expression* Cast::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    if (Node* node = mExpression->accept(visitor)) {
        mExpression.reset(static_cast<Expression*>(node));
    }
    return nullptr;
}

Expression* FunctionCall::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    if (Node* node = mArguments->accept(visitor)) {
        mArguments.reset(static_cast<ExpressionList*>(node));
    }
    return nullptr;
}

Expression* Return::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

Variable* Attribute::accept(Modifier& visitor)
{
    if (Variable* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

Expression* AttributeValue::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mAttribute->accept(visitor)) {
        mAttribute.reset(static_cast<Attribute*>(node));
    }
    return nullptr;
}

Variable* DeclareLocal::accept(Modifier& visitor)
{
    if (Variable* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

Variable* Local::accept(Modifier& visitor)
{
    if (Variable* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

Expression* LocalValue::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mLocal->accept(visitor)) {
        mLocal.reset(static_cast<Local*>(node));
    }
    return nullptr;
}

Expression* VectorUnpack::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mExpression->accept(visitor)) {
        mExpression.reset(static_cast<Expression*>(node));
    }
    return nullptr;
}

Expression* VectorPack::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mValue1->accept(visitor)) {
        mValue1.reset(static_cast<Expression*>(node));
    }
    if (Node* node = mValue2->accept(visitor)) {
        mValue2.reset(static_cast<Expression*>(node));
    }
    if (Node* node = mValue3->accept(visitor)) {
        mValue3.reset(static_cast<Expression*>(node));
    }

    return nullptr;
}

Expression* ArrayPack::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }

    if (Node* node = mArguments->accept(visitor)) {
        mArguments.reset(static_cast<ExpressionList*>(node));
    }
    return nullptr;
}

template <typename T>
Expression* Value<T>::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

Expression* Value<std::string>::accept(Modifier& visitor)
{
    if (Expression* node = visitor.visit(*this)) {
        return node;
    }
    return nullptr;
}

template struct Value<bool>;
template struct Value<int16_t>;
template struct Value<int32_t>;
template struct Value<int64_t>;
template struct Value<float>;
template struct Value<double>;

} // namespace ast
} // namespace ax

}
} // namespace openvdb


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


namespace {
// Declare this at file scope to ensure thread-safe initialization.
tbb::mutex sInitMutex;
std::string sLastParsingError;
}

using yyscan_t = void*;
using YY_BUFFER_STATE = struct yy_buffer_state*;

extern int yyparse(openvdb::ax::ast::Tree**);
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);

/// On a parsing error we store the error string in a global
/// variable so that we can access it later. Note that this does
/// not get called for invalid character lexical errors - we
/// immediate throw in the lexer in this case
extern void yyerror (openvdb::ax::ast::Tree**, char const *s) {
    sLastParsingError = std::string(s);
}

openvdb::ax::ast::Tree::Ptr
openvdb::ax::ast::parse(const char* code)
{
    tbb::mutex::scoped_lock lock(sInitMutex);

    YY_BUFFER_STATE buffer = yy_scan_string(code);

    openvdb::ax::ast::Tree* tree(nullptr);
    const int result = yyparse(&tree);

    openvdb::ax::ast::Tree::Ptr ptr(tree);

    yy_delete_buffer(buffer);

    if (result) {
        OPENVDB_THROW(openvdb::LLVMSyntaxError, sLastParsingError)
    }

    assert(ptr);
    return ptr;
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
