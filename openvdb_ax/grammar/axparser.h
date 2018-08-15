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
    IF = 262,
    ELSE = 263,
    RETURN = 264,
    EQUALS = 265,
    PLUSEQUALS = 266,
    MINUSEQUALS = 267,
    MULTIPLYEQUALS = 268,
    DIVIDEEQUALS = 269,
    PLUSPLUS = 270,
    MINUSMINUS = 271,
    LPARENS = 272,
    RPARENS = 273,
    LCURLY = 274,
    RCURLY = 275,
    PLUS = 276,
    MINUS = 277,
    MULTIPLY = 278,
    DIVIDE = 279,
    MODULO = 280,
    BITAND = 281,
    BITOR = 282,
    BITXOR = 283,
    BITNOT = 284,
    EQUALSEQUALS = 285,
    NOTEQUALS = 286,
    MORETHAN = 287,
    LESSTHAN = 288,
    MORETHANOREQUAL = 289,
    LESSTHANOREQUAL = 290,
    AND = 291,
    OR = 292,
    NOT = 293,
    STRING = 294,
    DOUBLE = 295,
    FLOAT = 296,
    LONG = 297,
    INT = 298,
    SHORT = 299,
    BOOL = 300,
    VOID = 301,
    F_AT = 302,
    I_AT = 303,
    V_AT = 304,
    S_AT = 305,
    COMMA = 306,
    VEC3I = 307,
    VEC3F = 308,
    VEC3D = 309,
    DOT_X = 310,
    DOT_Y = 311,
    DOT_Z = 312,
    L_SHORT = 313,
    L_INT = 314,
    L_LONG = 315,
    L_FLOAT = 316,
    L_DOUBLE = 317,
    L_STRING = 318,
    IDENTIFIER = 319,
    LPAREN = 320,
    RPAREN = 321,
    LOWER_THAN_ELSE = 322
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
    openvdb::ax::ast::Attribute* attribute;
    openvdb::ax::ast::AttributeValue* attributevalue;
    openvdb::ax::ast::DeclareLocal* declare_local;
    openvdb::ax::ast::Local* local;

#line 140 "grammar/axparser.h" /* yacc.c:1910  */
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
