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

#ifndef OPENVDB_AX_AST_HAS_BEEN_INCLUDED
#define OPENVDB_AX_AST_HAS_BEEN_INCLUDED

#include "Tokens.h"
#include "Literals.h"

#include <openvdb/Types.h>
#include <openvdb/util/Name.h>
#include <openvdb/version.h>

#include <memory>
#include <utility>
#include <vector>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

/// Forward declaration of the AST
struct Tree;

/// @brief  Construct an abstract syntax tree from a code snippet
///
std::shared_ptr<Tree> parse(const char* code);


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// Forward declarations of all AST nodes

struct Node;
struct Statement;
struct Block;
struct Expression;
struct ExpressionList;
struct ConditionalStatement;
struct AssignExpression;
struct Crement;
struct UnaryOperator;
struct BinaryOperator;
struct Cast;
struct Variable;
struct Attribute;
struct AttributeValue;
struct DeclareLocal;
struct Local;
struct LocalValue;
struct ValueBase;

struct Visitor;
struct Modifier;

// ----------------------------------------------------------------------------
// Visitor Objects
// ----------------------------------------------------------------------------

struct Node
{
    Node() = default;
    virtual ~Node() = 0;
    virtual void accept(Visitor&) const = 0;
    virtual Node* accept(Modifier&) = 0;
    virtual Node* copy() const = 0;
};

/* Concrete Nodes ------------------------------------------------------------- */

// Statements are anything that can make up a line, i.e. everything inbetween
// semicolons
struct Statement : public Node
{
    using Ptr = std::shared_ptr<Statement>;
    using UniquePtr = std::unique_ptr<Statement>;

    ~Statement() override = default;
    virtual Statement* copy() const override = 0;
};

// Expressions only contain identifiers, literals and operators, and can be
// reduced to some kind of value. For example:
//    3 + 5
//    min(3, 2)
struct Expression : public Statement
{
    using Ptr = std::shared_ptr<Expression>;
    using UniquePtr = std::unique_ptr<Expression>;

    ~Expression() override = default;
    virtual Expression* copy() const override = 0;
};

struct Block : public Statement
{
    using Ptr = std::shared_ptr<Block>;
    using UniquePtr = std::unique_ptr<Block>;

    Block() : mList() {}
    Block(const Block& other) : mList() {
        for (const Statement::Ptr& expr : other.mList) {
            mList.emplace_back(expr->copy());
        }
    }
    ~Block() override = default;

    void accept(Visitor& visitor) const override final;
    Block* accept(Modifier& visitor) override final;
    Block* copy() const override final { return new Block(*this); }

    std::vector<Statement::Ptr> mList;
};

// A tree is a list of statements that make up the whole program
struct Tree : public Node
{
    using Ptr = std::shared_ptr<Tree>;
    using ConstPtr = std::shared_ptr<const Tree>;
    using UniquePtr = std::unique_ptr<Tree>;

    Tree(Block* block) : mBlock(block) {}
    Tree() : mBlock(new Block()) {}
    Tree(const Tree& other) : mBlock(new Block(*other.mBlock)) {}
    ~Tree() override = default;

    void accept(Visitor& visitor) const override final;
    Tree* accept(Modifier& visitor) override final;
    Tree* copy() const override final { return new Tree(*this); }

    Block::Ptr mBlock;
};

struct ExpressionList : public Statement
{
    using Ptr = std::shared_ptr<ExpressionList>;
    using UniquePtr = std::unique_ptr<ExpressionList>;

    ExpressionList() : mList() {}
    ExpressionList(const ExpressionList& other)
        : mList() {
        for (const Expression::Ptr& expr : other.mList) {
            mList.emplace_back(expr->copy());
        }
    }
    ~ExpressionList() override = default;

    void accept(Visitor& visitor) const override final;
    ExpressionList* accept(Modifier& visitor) override final;
    ExpressionList* copy() const override final { return new ExpressionList(*this); }

    std::vector<Expression::Ptr> mList;
};

struct ConditionalStatement : public Statement
{
    using Ptr = std::shared_ptr<ConditionalStatement>;
    using UniquePtr = std::unique_ptr<ConditionalStatement>;

    ConditionalStatement(Expression* conditional,
                         Block* thenBranch,
                         Block* elseBranch)
        : mConditional(conditional)
        , mThenBranch(thenBranch)
        , mElseBranch(elseBranch) {}
    ConditionalStatement(const ConditionalStatement& other)
        : mConditional(other.mConditional->copy())
        , mThenBranch(new Block(*other.mThenBranch))
        , mElseBranch(new Block(*other.mElseBranch)) {}
    ~ConditionalStatement() override = default;

    void accept(Visitor& visitor) const override final;
    Statement* accept(Modifier& visitor) override final;
    ConditionalStatement* copy() const override final { return new ConditionalStatement(*this); }

    Expression::Ptr mConditional;
    Block::Ptr mThenBranch;
    Block::Ptr mElseBranch;
};

struct Variable : public Expression
{
    using Ptr = std::shared_ptr<Variable>;
    using UniquePtr = std::unique_ptr<Variable>;

    Variable(const Name& name) : mName(name) {}
    ~Variable() override = default;

    virtual Variable* copy() const override = 0;
    const Name mName;
};

// AssignExpression are of the form lvalue = expression
// Since they can be chained together, they are also expressions
struct AssignExpression : public Expression
{
    using Ptr = std::shared_ptr<AssignExpression>;
    using UniquePtr = std::unique_ptr<AssignExpression>;

    AssignExpression(Variable* variable,
                     Expression* expression)
        : mVariable(variable)
        , mExpression(expression) {}
    AssignExpression(const AssignExpression& other)
        : mVariable(other.mVariable->copy())
        , mExpression(other.mExpression->copy()) {}

    ~AssignExpression() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    AssignExpression* copy() const override final { return new AssignExpression(*this); }

    Variable::Ptr mVariable;
    Expression::Ptr mExpression;
};

struct Crement : public Expression
{
    using Ptr = std::shared_ptr<Crement>;
    using UniquePtr = std::unique_ptr<Crement>;

    enum Operation {
        Increment,
        Decrement
    };

    Crement(Variable* variable, Expression* expression, const Operation operation, bool post)
        : mVariable(variable)
        , mExpression(expression)
        , mOperation(operation)
        , mPost(post) {}
    Crement(const Crement& other)
        : mVariable(other.mVariable->copy())
        , mExpression(other.mExpression->copy())
        , mOperation(other.mOperation)
        , mPost(other.mPost) {}
    ~Crement() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    Crement* copy() const override final { return new Crement(*this); }

    Variable::Ptr mVariable;
    Expression::Ptr mExpression;
    const Operation mOperation;
    const bool mPost;
};

struct UnaryOperator : public Expression
{
    using Ptr = std::shared_ptr<UnaryOperator>;
    using UniquePtr = std::unique_ptr<UnaryOperator>;

    UnaryOperator(const std::string& op, Expression* expression)
        : mOperation(tokens::operatorTokenFromName(op))
        , mExpression(expression) {}
    UnaryOperator(const tokens::OperatorToken op, Expression* expression)
        : mOperation(op)
        , mExpression(expression) {}
    UnaryOperator(const UnaryOperator& other)
        : mOperation(other.mOperation)
        , mExpression(other.mExpression->copy()) {}
    ~UnaryOperator() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    UnaryOperator* copy() const override final { return new UnaryOperator(*this); }

    const tokens::OperatorToken mOperation;
    Expression::Ptr mExpression;
};

struct BinaryOperator : public Expression
{
    using Ptr = std::shared_ptr<BinaryOperator>;
    using UniquePtr = std::unique_ptr<BinaryOperator>;

    BinaryOperator(const std::string& op,
            Expression* left,
            Expression* right)
        : mOperation(tokens::operatorTokenFromName(op))
        , mLeft(left)
        , mRight(right) {}
    BinaryOperator(const tokens::OperatorToken op,
            Expression* left,
            Expression* right)
        : mOperation(op)
        , mLeft(left)
        , mRight(right) {}
    BinaryOperator(const BinaryOperator& other)
        : mOperation(other.mOperation)
        , mLeft(other.mLeft->copy())
        , mRight(other.mRight->copy()) {}
    ~BinaryOperator() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    BinaryOperator* copy() const override final { return new BinaryOperator(*this); }

    const tokens::OperatorToken mOperation;
    Expression::Ptr mLeft;
    Expression::Ptr mRight;
};

struct Cast : public Expression
{
    using Ptr = std::shared_ptr<Cast>;
    using UniquePtr = std::unique_ptr<Cast>;

    Cast(Expression* expression, const std::string& type)
        : mType(type)
        , mExpression(expression) {}
    Cast(const Cast& other)
        : mType(other.mType)
        , mExpression(other.mExpression->copy()) {}
    ~Cast() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    Cast* copy() const override final { return new Cast(*this); }

    const Name mType;
    Expression::Ptr mExpression;
};

struct FunctionCall : public Expression
{
    using Ptr = std::shared_ptr<FunctionCall>;
    using UniquePtr = std::unique_ptr<FunctionCall>;

    FunctionCall(const std::string& function)
        : mFunction(function)
        , mArguments(new ExpressionList()) {}
    FunctionCall(const std::string& function, ExpressionList* arguments)
        : mFunction(function)
        , mArguments(arguments) {}
    FunctionCall(const FunctionCall& other)
        : mFunction(other.mFunction)
        , mArguments(other.mArguments->copy()) {}
    ~FunctionCall() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    FunctionCall* copy() const override final { return new FunctionCall(*this); }

    const std::string mFunction;
    ExpressionList::Ptr mArguments;
};

struct Return : public Expression
{
    using Ptr = std::shared_ptr<Return>;
    using UniquePtr = std::unique_ptr<Return>;

    ~Return() override = default;
    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    Return* copy() const override final { return new Return(*this); }
};

struct Attribute : public Variable
{
    using Ptr = std::shared_ptr<Attribute>;
    using UniquePtr = std::unique_ptr<Attribute>;
    using NamePair = std::pair<Name, Name>;

    Attribute(const std::string& name, const std::string& type,
              const bool typeInferred = false)
        : Variable(name)
        , mType(type)
        , mTypeInferred(typeInferred) {}
    Attribute(const Attribute& other)
        : Variable(other.mName)
        , mType(other.mType)
        , mTypeInferred(other.mTypeInferred) {}
    ~Attribute() override = default;

    void accept(Visitor& visitor) const override final;
    Variable* accept(Modifier& visitor) override final;
    Attribute* copy() const override final { return new Attribute(*this); }

    const Name mType;
    const bool mTypeInferred;
};

struct AttributeValue : public Expression
{
    using Ptr = std::shared_ptr<AttributeValue>;
    using UniquePtr = std::unique_ptr<AttributeValue>;

    AttributeValue(Attribute* attribute)
        : mAttribute(attribute) {}
    AttributeValue(const AttributeValue& other)
        : mAttribute(new Attribute(*other.mAttribute)) {}
    ~AttributeValue() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    AttributeValue* copy() const override final { return new AttributeValue(*this); }

    Attribute::Ptr mAttribute;
};

struct DeclareLocal : public Variable
{
    using Ptr = std::shared_ptr<DeclareLocal>;
    using UniquePtr = std::unique_ptr<DeclareLocal>;

    DeclareLocal(const std::string& name, const std::string& type)
        : Variable(name)
        , mType(type) {}
    DeclareLocal(const DeclareLocal& other)
        : Variable(other.mName)
        , mType(other.mType) {}
    ~DeclareLocal() override = default;

    void accept(Visitor& visitor) const override final;
    Variable* accept(Modifier& visitor) override final;
    DeclareLocal* copy() const override final { return new DeclareLocal(*this); }

    const Name mType;
};

struct Local : public Variable
{
    using Ptr = std::shared_ptr<Local>;
    using UniquePtr = std::unique_ptr<Local>;

    Local(const std::string& name)
        : Variable(name) {}
    ~Local() override = default;

    void accept(Visitor& visitor) const override final;
    Variable* accept(Modifier& visitor) override final;
    Local* copy() const override final { return new Local(*this); }
};

struct LocalValue : public Expression
{
    using Ptr = std::shared_ptr<LocalValue>;
    using UniquePtr = std::unique_ptr<LocalValue>;

    LocalValue(Local* local)
        : mLocal(local) {}
    LocalValue(const LocalValue& other)
        : mLocal(new Local(*other.mLocal)) {}
    ~LocalValue() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    LocalValue* copy() const override final { return new LocalValue(*this); }

    Local::Ptr mLocal;
};

// ValueBases are a base class for anything that holds a value
// Derived classes store the actual typed values
struct ValueBase : public Expression
{
    ~ValueBase() override = default;
};

struct VectorUnpack : public Expression
{
    using Ptr = std::shared_ptr<VectorUnpack>;
    using UniquePtr = std::unique_ptr<VectorUnpack>;

    VectorUnpack(Expression* expression, const short index)
        : mExpression(expression)
        , mIndex(index) {}
    VectorUnpack(const VectorUnpack& other)
        : mExpression(other.mExpression->copy())
        , mIndex(other.mIndex) {}
    ~VectorUnpack() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    VectorUnpack* copy() const override final { return new VectorUnpack(*this); }

    Expression::Ptr mExpression;
    const short mIndex;
};

struct VectorPack : public ValueBase
{
    using Ptr = std::shared_ptr<VectorPack>;
    using UniquePtr = std::unique_ptr<VectorPack>;

    VectorPack(Expression* value1, Expression* value2, Expression* value3)
        : mValue1(value1), mValue2(value2), mValue3(value3) {}
    VectorPack(const VectorPack& other)
        : mValue1(other.mValue1->copy())
        , mValue2(other.mValue2->copy())
        , mValue3(other.mValue3->copy()) {}
    ~VectorPack() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    VectorPack* copy() const override final { return new VectorPack(*this); }

    Expression::Ptr mValue1;
    Expression::Ptr mValue2;
    Expression::Ptr mValue3;
};

struct ArrayPack : public ValueBase
{
    using Ptr = std::shared_ptr<ArrayPack>;
    using UniquePtr = std::unique_ptr<ArrayPack>;

    ArrayPack(ExpressionList* arguments)
        : mArguments(arguments) {}
    ArrayPack(const ArrayPack& other)
        : mArguments(new ExpressionList(*other.mArguments)) {}
    ~ArrayPack() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    ArrayPack* copy() const override final { return new ArrayPack(*this); }

    ExpressionList::Ptr mArguments;
};

template <typename T>
struct Value : public ValueBase
{
    using Ptr = std::shared_ptr<Value<T>>;
    using UniquePtr = std::unique_ptr<Value<T>>;

    using Type = T;
    using LiteralLimitsT = LiteralLimits<Type>;
    using ContainerType = typename LiteralLimitsT::ContainerT;

    /// @note   Value literals are always positive (negation is performed
    ///         as a unary operation)
    /// @brief  Literals store their value as ContainerType, which is guaranteed
    ///         to be at least large enough to represent the maximum possible
    ///         supported type for the requested precision.

    Value(const std::string number)
        : mValue(LiteralLimitsT::onLimitOverflow())
        , mText(nullptr) {
            try {
                mValue = LiteralLimitsT::convert(number);
            }
            catch (std::out_of_range&) {
                mText.reset(new std::string(number));
            }
        }
    Value(const ContainerType value)
        : mValue(value), mText(nullptr) {}

    Value(const Value<T>& other)
        : mValue(other.mValue)
        , mText(other.mText ? new std::string(*other.mText) : nullptr) {}

    ~Value() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    Value<Type>* copy() const override final { return new Value<Type>(*this); }

    // A container of a max size defined by LiteralValueContainer to hold values
    // which may be out of scope. This is only used for warnings
    ContainerType mValue;
    // The original string representation of the variable used for warnings if
    // it's unable to be represented (usually due to overflow)
    std::unique_ptr<std::string> mText;
};

template <>
struct Value<std::string> : public ValueBase
{
    using Type = std::string;
    Value(const Type& value) : mValue(value) {}
    Value(const Value<Type>& other) : mValue(other.mValue) {}
    ~Value() override = default;

    void accept(Visitor& visitor) const override final;
    Expression* accept(Modifier& visitor) override final;
    Value<Type>* copy() const override final { return new Value<Type>(*this); }

    const Type mValue;
};

// ----------------------------------------------------------------------------
// Visitor Objects
// ----------------------------------------------------------------------------

struct Visitor
{
    Visitor() = default;
    virtual ~Visitor() = 0;

    // Concrete Visitors can define a visit method for
    // any node type

    inline virtual void init(const Tree& node) {};
    inline virtual void visit(const Tree& node) {};
    inline virtual void visit(const Block& node) {};
    inline virtual void visit(const ExpressionList& node) {};
    inline virtual void visit(const ConditionalStatement& node) {};
    inline virtual void visit(const AssignExpression& node) {};
    inline virtual void visit(const Crement& node) {};
    inline virtual void visit(const UnaryOperator& node) {};
    inline virtual void visit(const BinaryOperator& node) {};
    inline virtual void visit(const Cast& node) {};
    inline virtual void visit(const FunctionCall& node) {};
    inline virtual void visit(const Return& node) {};
    inline virtual void visit(const Attribute& node) {};
    inline virtual void visit(const AttributeValue& node) {};
    inline virtual void visit(const DeclareLocal& node) {};
    inline virtual void visit(const Local& node) {};
    inline virtual void visit(const LocalValue& node) {};
    inline virtual void visit(const VectorUnpack& node) {};
    inline virtual void visit(const VectorPack& node) {};
    inline virtual void visit(const ArrayPack& node) {};
    inline virtual void visit(const Value<bool>& node) {};
    inline virtual void visit(const Value<int16_t>& node) {};
    inline virtual void visit(const Value<int32_t>& node) {};
    inline virtual void visit(const Value<int64_t>& node) {};
    inline virtual void visit(const Value<float>& node) {};
    inline virtual void visit(const Value<double>& node) {};
    inline virtual void visit(const Value<std::string>& node) {};
};


struct Modifier
{
    Modifier() = default;
    virtual ~Modifier() = 0;

    inline virtual void visit(Tree& node) {};
    inline virtual Block* visit(Block& node) { return nullptr; };
    inline virtual ExpressionList* visit(ExpressionList& node) { return nullptr; };
    inline virtual Statement*  visit(ConditionalStatement& node) { return nullptr; };
    inline virtual Expression* visit(AssignExpression& node) { return nullptr; };
    inline virtual Expression* visit(Crement& node) { return nullptr; };
    inline virtual Expression* visit(UnaryOperator& node) { return nullptr; };
    inline virtual Expression* visit(BinaryOperator& node) { return nullptr; };
    inline virtual Expression* visit(Cast& node) { return nullptr; };
    inline virtual Expression* visit(FunctionCall& node) { return nullptr; };
    inline virtual Expression* visit(Return& node) { return nullptr; };
    inline virtual Variable*   visit(Attribute& node) { return nullptr; };
    inline virtual Expression* visit(AttributeValue& node) { return nullptr; };
    inline virtual Variable*   visit(DeclareLocal& node) { return nullptr; };
    inline virtual Variable*   visit(Local& node) { return nullptr; };
    inline virtual Expression* visit(LocalValue& node) { return nullptr; };
    inline virtual Expression* visit(VectorUnpack& node) { return nullptr; };
    inline virtual Expression* visit(VectorPack& node) { return nullptr; };
    inline virtual Expression* visit(ArrayPack& node) { return nullptr; };
    inline virtual Expression* visit(Value<bool>& node) { return nullptr; };
    inline virtual Expression* visit(Value<int16_t>& node) { return nullptr; };
    inline virtual Expression* visit(Value<int32_t>& node) { return nullptr; };
    inline virtual Expression* visit(Value<int64_t>& node) { return nullptr; };
    inline virtual Expression* visit(Value<float>& node) { return nullptr; };
    inline virtual Expression* visit(Value<double>& node) { return nullptr; };
    inline virtual Expression* visit(Value<std::string>& node) { return nullptr; };
};

} // namespace ast
} // namespace ax

}
} // namespace openvdb

extern FILE* yyin;
extern int yyparse(openvdb::ax::ast::Tree** tree);

#endif // OPENVDB_AX_AST_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
