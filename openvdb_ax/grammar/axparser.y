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

%{
    #include <stdio.h>
    #include <iostream>

    #include <openvdb_ax/ast/AST.h>
    #include <openvdb_ax/ast/Tokens.h>

    extern int yylex(void);

    using namespace openvdb::ax::ast;

    void yyerror(Tree** tree, const char* s);

    // @note  Component expressions are currently fully unpacked, processed, packed, and then
    //        vector assigned. This should not be the case for locals.
    // @todo  Fix this by allowing direct assignment to local array elements and better
    //        handling of attribute elements
    Expression*
    buildComponentExpression(const std::unique_ptr<tokens::OperatorToken> op,
                             Variable::UniquePtr lhs,         // The attribute/local vector value being assigned (Attribute/Local)
                             Expression::UniquePtr lhsValue,  // The attribute/local vector value being assigned (AttibuteValue/LocalValue)
                             const int16_t index,
                             Expression::UniquePtr rhs)       // The RHS expression
    {
        assert(index >= 0 && index <= 2);

        if (op) {
            rhs.reset(new BinaryOperator(*op, new VectorUnpack(lhsValue->copy(), index), rhs.release()));
        }

        Expression::UniquePtr elements[3];
        if (index == 0) elements[0].reset(rhs.release());
        else            elements[0].reset(new VectorUnpack(lhsValue->copy(), 0));
        if (index == 1) elements[1].reset(rhs.release());
        else            elements[1].reset(new VectorUnpack(lhsValue->copy(), 1));
        if (index == 2) elements[2].reset(rhs.release());
        else            elements[2].reset(new VectorUnpack(lhsValue->copy(), 2));

        VectorPack::UniquePtr packed(new VectorPack(elements[0].release(), elements[1].release(), elements[2].release()));
        return new AssignExpression(lhs.release(), packed.release());
    }

    Expression*
    buildAttributeComponentExpression(tokens::OperatorToken* op,
                                      Attribute* attribute,
                                      const int16_t index,
                                      Expression* expression)
    {
        return buildComponentExpression(std::unique_ptr<tokens::OperatorToken>(op),
            Variable::UniquePtr(attribute),
            Expression::UniquePtr(new AttributeValue(attribute->copy())),
            index,
            Expression::UniquePtr(expression));
    }

    Expression* buildLocalComponentExpression(tokens::OperatorToken* op,
                                              Local* local,
                                              const int16_t index,
                                              Expression* expression)
    {
        return buildComponentExpression(std::unique_ptr<tokens::OperatorToken>(op),
            Variable::UniquePtr(local),
            Expression::UniquePtr(new LocalValue(local->copy())),
            index,
            Expression::UniquePtr(expression));
    }

    Attribute* buildAttribute(const std::string& name)
    {
        // remap position, velocity, normal and color to vec3s
        if (name == "P" || name == "v" || name == "N" || name == "Cd") {
            return new Attribute(name, openvdb::typeNameAsString<openvdb::Vec3s>());
        }
        else {
            return new Attribute(name, openvdb::typeNameAsString<float>());
        }
    }
%}

%error-verbose
%locations

%union
{
    const char* value_string;
    uint64_t index;

    openvdb::ax::ast::Tree* tree;
    openvdb::ax::ast::ValueBase* value;
    openvdb::ax::ast::VectorUnpack* vector_unpack;
    openvdb::ax::ast::Block* block;
    openvdb::ax::ast::Statement* statement;
    openvdb::ax::ast::Expression* expression;
    openvdb::ax::ast::ExpressionList* expressionlist;
    openvdb::ax::ast::Variable* variable;
    openvdb::ax::ast::ExternalVariable* external;
    openvdb::ax::ast::Attribute* attribute;
    openvdb::ax::ast::AttributeValue* attributevalue;
    openvdb::ax::ast::DeclareLocal* declare_local;
    openvdb::ax::ast::Local* local;
}

%start statements

%token TRUE FALSE
%token SEMICOLON AT DOLLAR
%token IF ELSE
%token RETURN
%token EQUALS PLUSEQUALS MINUSEQUALS MULTIPLYEQUALS DIVIDEEQUALS PLUSPLUS MINUSMINUS
%token LPARENS RPARENS LCURLY RCURLY
%token PLUS MINUS MULTIPLY DIVIDE MODULO
%token BITAND BITOR BITXOR BITNOT
%token EQUALSEQUALS NOTEQUALS MORETHAN LESSTHAN MORETHANOREQUAL LESSTHANOREQUAL
%token AND OR NOT
%token STRING DOUBLE FLOAT LONG INT SHORT BOOL VOID F_AT I_AT V_AT S_AT F_DOLLAR I_DOLLAR V_DOLLAR S_DOLLAR
%token COMMA
%token VEC3I VEC3F VEC3D
%token DOT_X DOT_Y DOT_Z

%token <value_string> L_SHORT
%token <value_string> L_INT
%token <value_string> L_LONG
%token <value_string> L_FLOAT
%token <value_string> L_DOUBLE
%token <value_string> L_STRING
%token <value_string> IDENTIFIER

%type <tree> statements
%type <block> block
%type <block> body
%type <statement> statement
%type <statement> conditional_statement
%type <statement> declare_assignment
%type <expression> assign_expression
%type <expression> assign_component_expression
%type <expression> crement
%type <expression> expression
%type <expression> expression_expand
%type <expression> function_call_expression
%type <expression> binary_expression
%type <expression> unary_expression
%type <expression> cast_expression
%type <expressionlist> arguments
%type <attribute> attribute
%type <external> external
%type <declare_local> declare_local
%type <local> local
%type <vector_unpack> vector_element
%type <index> component

%type <value> vector_literal
%type <value> literal
%type <value_string> scalar_type
%type <value_string> vector_type

%right EQUALS PLUSEQUALS MINUSEQUALS MULTIPLYEQUALS DIVIDEEQUALS PLUSPLUS MINUSMINUS
%left AND OR
%right NOT
%right EQUALSEQUALS NOTEQUALS
%left MORETHAN LESSTHAN MORETHANOREQUAL LESSTHANOREQUAL
%left BITAND BITOR BITXOR
%left PLUS MINUS
%left MULTIPLY DIVIDE
%left MODULO
%right BITNOT
%left LPAREN RPAREN

%nonassoc LOWER_THAN_ELSE //null token to force no associativity in conditional statement
%nonassoc ELSE

%parse-param {openvdb::ax::ast::Tree** tree}

%%

statements:
      { *tree = new Tree(); $$ = *tree; } /* nothing */
    | body { *tree = new Tree($1); $$ = *tree; }
;

block:
      LCURLY body RCURLY  { $$ = $2; }
    | LCURLY RCURLY       { $$ = new Block(); }
    | statement           { $$ = new Block(); if($1) $$->mList.emplace_back($1); }
;

body:
      body statement  { if ($2) $1->mList.emplace_back($2); $$ = $1; }
    | statement       { $$ = new Block(); if ($1) $$->mList.emplace_back($1); }
;

/// @brief  Syntax for a statement; a line followed by a semicolon or a
///         conditional statement
statement:
      expression SEMICOLON          { $$ = $1; }
    | declare_assignment SEMICOLON  { $$ = $1; }
    | conditional_statement         { $$ = $1; }
    | RETURN SEMICOLON              { $$ = new Return; }
    | SEMICOLON                     { $$ = nullptr; } // The only possible nullptr rule
;

/// @brief  Syntax for a conditional statement, capable of supporting a single if
///         and an optional single else clause
/// @todo   Support multiple else if statements
/// @todo   Support break keyword
conditional_statement:
      IF expression_expand block %prec LOWER_THAN_ELSE  { $$ = new ConditionalStatement($2, $3, new Block()); }
    | IF expression_expand block ELSE block             { $$ = new ConditionalStatement($2, $3, $5); }
;

/// @brief  Syntax for a combination of all numerical expressions which can return
///         an rvalue.
expression:
      expression_expand            { $$ = $1; }
    | function_call_expression     { $$ = $1; }
    | binary_expression            { $$ = $1; }
    | unary_expression             { $$ = $1; }
    | assign_expression            { $$ = $1; }
    | assign_component_expression  { $$ = $1; }
    | crement                      { $$ = $1; }
    | cast_expression              { $$ = $1; }
    | vector_literal               { $$ = $1; }
    | vector_element               { $$ = $1; }
    | literal                      { $$ = $1; }
    | external                     { $$ = $1; }
    | local                        { $$ = new LocalValue($1); }
    | attribute                    { $$ = new AttributeValue($1); }
;

/// @brief  Component access to vectors
/// @note   The result of this access is always an rvalue i.e. a non writable
///         value.
vector_element:
    attribute component  { $$ = new VectorUnpack(new AttributeValue($1), $2); } |
    local component      { $$ = new VectorUnpack(new LocalValue($1), $2); }     |
    external component   { $$ = new VectorUnpack($1, $2); }
;

/// @brief  A unique type of expression to support recursive brackets
expression_expand:
    LPARENS expression RPARENS  { $$ = $2; }
;

/// @brief  A unique type for cast expressions
/// @note   Only scalar casts are currently supported
/// @todo   Extend to vector and string casts
cast_expression:
    scalar_type expression_expand  { $$ = new Cast($2, $1); }
;

/// @brief  A function call, taking zero or any arguments
function_call_expression:
      IDENTIFIER LPARENS arguments RPARENS  { $$ = new FunctionCall($1, $3); free((char*)$1); }
    | IDENTIFIER LPARENS RPARENS            { $$ = new FunctionCall($1); free((char*)$1); }
;

/// @brief  An argument list of at least size 1
arguments:
      expression                  { $$ = new ExpressionList(); $$->mList.emplace_back($1); }
    | arguments COMMA expression  { $1->mList.emplace_back($3); $$ = $1; }
;

/// @brief  The declaration of local variables. Unlike assign expression which
///         return values (for assignment chains), assign declarations must end
///         in a statement.
declare_assignment:
      declare_local EQUALS expression  { $$ = new AssignExpression($1, $3); }
    | declare_local                    { $$ = $1; }
;

/// @brief  Non component assign expressions for attributes and local variables
/// @note   Currently a single AST node supports local and attribute assignments.
///         This could be split out into unique types to represent both. This may
///         make consolidation with component assignments easier
/// @todo   Consolidate with component assignments
assign_expression:
      attribute EQUALS expression          { $$ = new AssignExpression($1, $3); }
    | attribute PLUSEQUALS expression      { $$ = new AssignExpression($1, new BinaryOperator(tokens::PLUS, new AttributeValue($1->copy()), $3)); }
    | attribute MINUSEQUALS expression     { $$ = new AssignExpression($1, new BinaryOperator(tokens::MINUS, new AttributeValue($1->copy()), $3)); }
    | attribute MULTIPLYEQUALS expression  { $$ = new AssignExpression($1, new BinaryOperator(tokens::MULTIPLY, new AttributeValue($1->copy()), $3)); }
    | attribute DIVIDEEQUALS expression    { $$ = new AssignExpression($1, new BinaryOperator(tokens::DIVIDE, new AttributeValue($1->copy()), $3)); }
    | local EQUALS expression              { $$ = new AssignExpression($1, $3); }
    | local PLUSEQUALS expression          { $$ = new AssignExpression($1, new BinaryOperator(tokens::PLUS, new LocalValue($1->copy()), $3)); }
    | local MINUSEQUALS expression         { $$ = new AssignExpression($1, new BinaryOperator(tokens::MINUS, new LocalValue($1->copy()), $3)); }
    | local MULTIPLYEQUALS expression      { $$ = new AssignExpression($1, new BinaryOperator(tokens::MULTIPLY, new LocalValue($1->copy()), $3)); }
    | local DIVIDEEQUALS expression        { $$ = new AssignExpression($1, new BinaryOperator(tokens::DIVIDE, new LocalValue($1->copy()), $3)); }
;

/// @brief  Component assign expressions for attributes and local variables
/// @note   Due to the way attributes are passed around (void* to a handle)
///         and due to the function implementation of attribute writes, we must
///         pull out the entire value, modify the component, and then re-assign to
///         perform component assignments. This is not necessary for locals.
/// @todo   Simplify component assignments for local variables and introduce flexible
///         solution for attribute component assignments
assign_component_expression:
      attribute component EQUALS expression          { $$ = buildAttributeComponentExpression(nullptr, $1, $2, $4); }
    | attribute component PLUSEQUALS expression      { $$ = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::PLUS), $1, $2, $4); }
    | attribute component MINUSEQUALS expression     { $$ = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MINUS), $1, $2, $4); }
    | attribute component MULTIPLYEQUALS expression  { $$ = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), $1, $2, $4); }
    | attribute component DIVIDEEQUALS expression    { $$ = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), $1, $2, $4); }
    | local component EQUALS expression              { $$ = buildLocalComponentExpression(nullptr, $1, $2, $4); }
    | local component PLUSEQUALS expression          { $$ = buildLocalComponentExpression(new tokens::OperatorToken(tokens::PLUS), $1, $2, $4); }
    | local component MINUSEQUALS expression         { $$ = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MINUS), $1, $2, $4); }
    | local component MULTIPLYEQUALS expression      { $$ = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), $1, $2, $4); }
    | local component DIVIDEEQUALS expression        { $$ = buildLocalComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), $1, $2, $4); }
;

/// @brief  Attribute and local crement operations.
/// @todo   Support component crement and properly handle pre and post crement return
///         types. That is, pre crement should return an assignable lvalue, where as
///         post crement should return an rvalue to the cremented value. This is complex
///         for attributes.
crement:
      PLUSPLUS attribute    { $$ = new Crement($2, new AttributeValue($2->copy()), Crement::Increment, /*post*/false); }
    | MINUSMINUS attribute  { $$ = new Crement($2, new AttributeValue($2->copy()), Crement::Decrement, /*post*/false); }
    | attribute PLUSPLUS    { $$ = new Crement($1, new AttributeValue($1->copy()), Crement::Increment, /*post*/true); }
    | attribute MINUSMINUS  { $$ = new Crement($1, new AttributeValue($1->copy()), Crement::Decrement, /*post*/true); }
    | PLUSPLUS local        { $$ = new Crement($2, new LocalValue($2->copy()), Crement::Increment, /*post*/false); }
    | MINUSMINUS local      { $$ = new Crement($2, new LocalValue($2->copy()), Crement::Decrement, /*post*/false); }
    | local PLUSPLUS        { $$ = new Crement($1, new LocalValue($1->copy()), Crement::Increment, /*post*/true); }
    | local MINUSMINUS      { $$ = new Crement($1, new LocalValue($1->copy()), Crement::Decrement, /*post*/true); }
;

/// @brief  A unary expression which takes an expression and returns an expression
unary_expression:
      PLUS expression    { $$ = new UnaryOperator(tokens::PLUS, $2); }
    | MINUS expression   { $$ = new UnaryOperator(tokens::MINUS, $2); }
    | BITNOT expression  { $$ = new UnaryOperator(tokens::BITNOT, $2); }
    | NOT expression     { $$ = new UnaryOperator(tokens::NOT, $2); }
;

/// @brief  A binary expression which takes a left and right hand side expression
///         and returns an expression
binary_expression:
      expression PLUS expression             { $$ = new BinaryOperator(tokens::PLUS, $1, $3); }
    | expression MINUS expression            { $$ = new BinaryOperator(tokens::MINUS, $1, $3); }
    | expression MULTIPLY expression         { $$ = new BinaryOperator(tokens::MULTIPLY, $1, $3); }
    | expression DIVIDE expression           { $$ = new BinaryOperator(tokens::DIVIDE, $1, $3); }
    | expression MODULO expression           { $$ = new BinaryOperator(tokens::MODULO, $1, $3); }
    | expression BITAND expression           { $$ = new BinaryOperator(tokens::BITAND, $1, $3); }
    | expression BITOR expression            { $$ = new BinaryOperator(tokens::BITOR, $1, $3); }
    | expression BITXOR expression           { $$ = new BinaryOperator(tokens::BITXOR, $1, $3); }
    | expression AND expression              { $$ = new BinaryOperator(tokens::AND, $1, $3); }
    | expression OR expression               { $$ = new BinaryOperator(tokens::OR, $1, $3); }
    | expression EQUALSEQUALS expression     { $$ = new BinaryOperator(tokens::EQUALSEQUALS, $1, $3); }
    | expression NOTEQUALS expression        { $$ = new BinaryOperator(tokens::NOTEQUALS, $1, $3); }
    | expression MORETHAN expression         { $$ = new BinaryOperator(tokens::MORETHAN, $1, $3); }
    | expression LESSTHAN expression         { $$ = new BinaryOperator(tokens::LESSTHAN, $1, $3); }
    | expression MORETHANOREQUAL expression  { $$ = new BinaryOperator(tokens::MORETHANOREQUAL, $1, $3); }
    | expression LESSTHANOREQUAL expression  { $$ = new BinaryOperator(tokens::LESSTHANOREQUAL, $1, $3); }
;

/// @brief  Syntax for vector literals
vector_literal:
    LCURLY expression COMMA expression COMMA expression RCURLY { $$ = new VectorPack($2, $4, $6); }
;

/// @brief  Syntax for supported attribute access
attribute:
      scalar_type AT IDENTIFIER  { $$ = new Attribute($3, $1); free((char*)$3); }
    | vector_type AT IDENTIFIER  { $$ = new Attribute($3, $1); free((char*)$3); }
    | I_AT  IDENTIFIER           { $$ = new Attribute($2, openvdb::typeNameAsString<int32_t>()); free((char*)$2); }
    | F_AT  IDENTIFIER           { $$ = new Attribute($2, openvdb::typeNameAsString<float>()); free((char*)$2); }
    | V_AT IDENTIFIER            { $$ = new Attribute($2, openvdb::typeNameAsString<openvdb::Vec3s>()); free((char*)$2); }
    | S_AT IDENTIFIER            { $$ = new Attribute($2, openvdb::typeNameAsString<std::string>()); free((char*)$2); }
    | STRING AT IDENTIFIER       { $$ = new Attribute($3, openvdb::typeNameAsString<std::string>()); free((char*)$3); }
    | AT IDENTIFIER              { $$ = new Attribute($2, openvdb::typeNameAsString<float>(), true); free((char*)$2); }
;

/// @brief  Syntax for supported external variable access
external:
      scalar_type DOLLAR IDENTIFIER  { $$ = new ExternalVariable($3, $1); free((char*)$3); }
    | vector_type DOLLAR IDENTIFIER  { $$ = new ExternalVariable($3, $1); free((char*)$3); }
    | I_DOLLAR  IDENTIFIER           { $$ = new ExternalVariable($2, openvdb::typeNameAsString<int32_t>()); free((char*)$2); }
    | F_DOLLAR  IDENTIFIER           { $$ = new ExternalVariable($2, openvdb::typeNameAsString<float>()); free((char*)$2); }
    | V_DOLLAR IDENTIFIER            { $$ = new ExternalVariable($2, openvdb::typeNameAsString<openvdb::Vec3s>()); free((char*)$2); }
    | S_DOLLAR IDENTIFIER            { $$ = new ExternalVariable($2, openvdb::typeNameAsString<std::string>()); free((char*)$2); }
    | STRING DOLLAR IDENTIFIER       { $$ = new ExternalVariable($3, openvdb::typeNameAsString<std::string>()); free((char*)$3); }
    | DOLLAR IDENTIFIER              { $$ = new ExternalVariable($2, openvdb::typeNameAsString<float>()); free((char*)$2); }
;

/// @brief  Syntax for the declaration of supported local variable types
declare_local:
      scalar_type IDENTIFIER  { $$ = new DeclareLocal($2, $1); free((char*)$2); }
    | vector_type IDENTIFIER  { $$ = new DeclareLocal($2, $1); free((char*)$2); }
    | STRING IDENTIFIER       { $$ = new DeclareLocal($2, openvdb::typeNameAsString<std::string>()); free((char*)$2); }
;

/// @brief  Syntax for text identifiers which resolves to a local. Types have
///         have their own tokens which do not evaluate to a local variable
/// @note   Anything which uses an IDENTIFIER must free the returned char array
local:
    IDENTIFIER  { $$ = new Local($1); free((char*)$1); }
;

/// @brief  Syntax numerical and boolean literal values
/// @note   Anything which uses one of the below tokens must free the returned char
///         array (aside from TRUE and FALSE tokens)
literal:
      L_SHORT   { $$ = new Value<int16_t>($1); free((char*)$1); }
    | L_INT     { $$ = new Value<int32_t>($1); free((char*)$1); }
    | L_LONG    { $$ = new Value<int64_t>($1); free((char*)$1); }
    | L_FLOAT   { $$ = new Value<float>($1); free((char*)$1); }
    | L_DOUBLE  { $$ = new Value<double>($1); free((char*)$1); }
    | L_STRING  { $$ = new Value<std::string>($1); free((char*)$1); }
    | TRUE      { $$ = new Value<bool>(true); }
    | FALSE     { $$ = new Value<bool>(false); }
;

/// @brief  Vector component access
component:
      DOT_X  { $$ = 0; }
    | DOT_Y  { $$ = 1; }
    | DOT_Z  { $$ = 2; }
;

/// @brief  Scalar types consolidated as strings. These should be used to ensure
///         type matching.
scalar_type:
      BOOL    { $$ = openvdb::typeNameAsString<bool>(); }
    | SHORT   { $$ = openvdb::typeNameAsString<int16_t>(); }
    | INT     { $$ = openvdb::typeNameAsString<int32_t>(); }
    | LONG    { $$ = openvdb::typeNameAsString<int64_t>(); }
    | FLOAT   { $$ = openvdb::typeNameAsString<float>(); }
    | DOUBLE  { $$ = openvdb::typeNameAsString<double>(); }
;

/// @brief  Vector types consolidated as strings. These should be used to ensure
///         type matching.
vector_type:
      VEC3I   { $$ = openvdb::typeNameAsString<openvdb::Vec3i>(); }
    | VEC3F   { $$ = openvdb::typeNameAsString<openvdb::Vec3s>(); }
    | VEC3D   { $$ = openvdb::typeNameAsString<openvdb::Vec3d>(); }

%%

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
