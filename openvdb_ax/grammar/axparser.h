/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_GRAMMAR_AXPARSER_H_INCLUDED
# define YY_YY_GRAMMAR_AXPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TRUE = 258,
    FALSE = 259,
    SEMICOLON = 260,
    AT = 261,
    DOLLAR = 262,
    IF = 263,
    ELSE = 264,
    RETURN = 265,
    EQUALS = 266,
    PLUSEQUALS = 267,
    MINUSEQUALS = 268,
    MULTIPLYEQUALS = 269,
    DIVIDEEQUALS = 270,
    PLUSPLUS = 271,
    MINUSMINUS = 272,
    LPARENS = 273,
    RPARENS = 274,
    LCURLY = 275,
    RCURLY = 276,
    PLUS = 277,
    MINUS = 278,
    MULTIPLY = 279,
    DIVIDE = 280,
    MODULO = 281,
    BITAND = 282,
    BITOR = 283,
    BITXOR = 284,
    BITNOT = 285,
    EQUALSEQUALS = 286,
    NOTEQUALS = 287,
    MORETHAN = 288,
    LESSTHAN = 289,
    MORETHANOREQUAL = 290,
    LESSTHANOREQUAL = 291,
    AND = 292,
    OR = 293,
    NOT = 294,
    STRING = 295,
    DOUBLE = 296,
    FLOAT = 297,
    LONG = 298,
    INT = 299,
    SHORT = 300,
    BOOL = 301,
    VOID = 302,
    F_AT = 303,
    I_AT = 304,
    V_AT = 305,
    S_AT = 306,
    F_DOLLAR = 307,
    I_DOLLAR = 308,
    V_DOLLAR = 309,
    S_DOLLAR = 310,
    COMMA = 311,
    VEC3I = 312,
    VEC3F = 313,
    VEC3D = 314,
    DOT_X = 315,
    DOT_Y = 316,
    DOT_Z = 317,
    L_SHORT = 318,
    L_INT = 319,
    L_LONG = 320,
    L_FLOAT = 321,
    L_DOUBLE = 322,
    L_STRING = 323,
    IDENTIFIER = 324,
    LPAREN = 325,
    RPAREN = 326,
    LOWER_THAN_ELSE = 327
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 114 "grammar/axparser.y" /* yacc.c:1910  */

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

#line 146 "grammar/axparser.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;
int yyparse (openvdb::ax::ast::Tree** tree);

#endif /* !YY_YY_GRAMMAR_AXPARSER_H_INCLUDED  */
