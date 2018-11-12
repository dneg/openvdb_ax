/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 31 "grammar/axparser.y" /* yacc.c:339  */

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

#line 145 "grammar/axparser.cc" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "axparser.h".  */
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
#line 114 "grammar/axparser.y" /* yacc.c:355  */

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

#line 277 "grammar/axparser.cc" /* yacc.c:355  */
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

/* Copy the second part of user declarations.  */

#line 308 "grammar/axparser.cc" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  97
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   729

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  73
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  129
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  219

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   327

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   206,   206,   207,   211,   212,   213,   217,   218,   224,
     225,   226,   227,   228,   236,   237,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     263,   264,   265,   270,   277,   282,   283,   288,   289,   296,
     297,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   344,   345,   346,   347,   348,   349,   350,   351,   356,
     357,   358,   359,   365,   366,   367,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   385,
     390,   391,   392,   393,   394,   395,   396,   397,   402,   403,
     404,   405,   406,   407,   408,   409,   414,   415,   416,   423,
     430,   431,   432,   433,   434,   435,   436,   437,   442,   443,
     444,   450,   451,   452,   453,   454,   455,   461,   462,   463
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TRUE", "FALSE", "SEMICOLON", "AT",
  "DOLLAR", "IF", "ELSE", "RETURN", "EQUALS", "PLUSEQUALS", "MINUSEQUALS",
  "MULTIPLYEQUALS", "DIVIDEEQUALS", "PLUSPLUS", "MINUSMINUS", "LPARENS",
  "RPARENS", "LCURLY", "RCURLY", "PLUS", "MINUS", "MULTIPLY", "DIVIDE",
  "MODULO", "BITAND", "BITOR", "BITXOR", "BITNOT", "EQUALSEQUALS",
  "NOTEQUALS", "MORETHAN", "LESSTHAN", "MORETHANOREQUAL",
  "LESSTHANOREQUAL", "AND", "OR", "NOT", "STRING", "DOUBLE", "FLOAT",
  "LONG", "INT", "SHORT", "BOOL", "VOID", "F_AT", "I_AT", "V_AT", "S_AT",
  "F_DOLLAR", "I_DOLLAR", "V_DOLLAR", "S_DOLLAR", "COMMA", "VEC3I",
  "VEC3F", "VEC3D", "DOT_X", "DOT_Y", "DOT_Z", "L_SHORT", "L_INT",
  "L_LONG", "L_FLOAT", "L_DOUBLE", "L_STRING", "IDENTIFIER", "LPAREN",
  "RPAREN", "LOWER_THAN_ELSE", "$accept", "statements", "block", "body",
  "statement", "conditional_statement", "expression", "vector_element",
  "expression_expand", "cast_expression", "function_call_expression",
  "arguments", "declare_assignment", "assign_expression",
  "assign_component_expression", "crement", "unary_expression",
  "binary_expression", "vector_literal", "attribute", "external",
  "declare_local", "local", "literal", "component", "scalar_type",
  "vector_type", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327
};
# endif

#define YYPACT_NINF -61

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-61)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     314,   -61,   -61,   -61,   -60,   -52,     4,    43,   545,   545,
     515,   515,   515,   515,   515,   515,    14,   -61,   -61,   -61,
     -61,   -61,   -61,   -43,   -40,   -18,     6,    12,    16,    37,
      45,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,
     100,   138,   314,   -61,   -61,   636,   -61,   -61,   -61,   -61,
     145,   -61,   -61,   -61,   -61,   -61,   -61,    55,     1,   123,
     114,   -61,     5,    18,   -61,   -61,   381,   -61,   147,   -61,
     -61,   -61,   148,   150,   -61,   -61,    82,   656,    21,   106,
     584,    53,    53,   -61,   620,    88,    89,   -61,   -61,   -61,
     -61,   -61,   -61,   -61,   -61,   -61,   448,   -61,   -61,   -61,
     515,   515,   515,   515,   515,   515,   515,   515,   515,   515,
     515,   515,   515,   515,   515,   515,   -61,   515,   515,   515,
     515,   515,   -61,   -61,   -61,   -61,   -61,   129,   -61,   515,
     515,   515,   515,   515,   515,   -61,   -61,   134,    90,    91,
     -61,   -61,    93,    95,   -61,   180,   142,   -61,   -61,   515,
     -61,   -61,   -61,   691,    -6,    53,    53,   139,   139,   -61,
     189,   189,   189,   620,   620,    31,    31,    31,    31,   620,
     620,   691,   691,   691,   691,   691,   515,   515,   515,   515,
     515,   691,   691,   691,   691,   691,   691,   515,   515,   515,
     515,   515,   -61,   -61,   -61,   -61,   -61,   247,     9,   381,
     601,   -61,   515,   691,   691,   691,   691,   691,   691,   691,
     691,   691,   691,   -61,   -61,   515,   691,   674,   -61
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,   116,   117,    13,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   126,   125,   124,
     123,   122,   121,     0,     0,     0,     0,     0,     0,     0,
       0,   127,   128,   129,   110,   111,   112,   113,   114,   115,
     109,     0,     3,     8,    11,     0,    25,    16,    23,    17,
       0,    20,    21,    22,    19,    18,    24,    29,    27,    40,
      28,    26,     0,     0,    97,   105,     0,    12,     0,   109,
      61,    65,     0,     0,    62,    66,     0,     0,     0,     0,
       0,    69,    70,    71,    72,     0,     0,   108,    93,    92,
      94,    95,   101,   100,   102,   103,     0,     1,     7,     9,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    10,     0,     0,     0,
       0,     0,    63,    64,   118,   119,   120,    30,    32,     0,
       0,     0,     0,     0,     0,    67,    68,    31,     0,     0,
     106,    34,     0,     0,   107,     0,    14,     6,    33,     0,
      96,   104,    36,    37,     0,    73,    74,    75,    76,    77,
      78,    79,    80,    83,    84,    85,    86,    87,    88,    81,
      82,    41,    42,    43,    44,    45,     0,     0,     0,     0,
       0,    39,    46,    47,    48,    49,    50,     0,     0,     0,
       0,     0,    90,    98,    91,    99,     5,     0,     0,     0,
       0,    35,     0,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,     4,    15,     0,    38,     0,    89
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -61,   -61,   -28,    27,   -36,   -61,   -10,   -61,     2,   -61,
     -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   -61,   124,
     -61,   -61,   128,   -61,    24,     7,    10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    41,   146,    42,    43,    44,    45,    46,    47,    48,
      49,   154,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,   127,    78,    79
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      77,    80,    81,    82,    83,    84,    98,    62,    66,    64,
      63,   138,   139,   201,    99,    72,    72,    65,    73,    73,
      85,    86,    10,    10,   142,   143,    88,   138,   139,    89,
     147,   100,   101,   102,   103,   104,   105,   106,   107,    10,
     108,   109,   110,   111,   112,   113,   114,   115,    67,    62,
     202,    90,    63,   100,   101,   102,   103,   104,   105,   106,
     107,   124,   125,   126,   141,   149,   117,   118,   119,   120,
     121,   122,   123,    62,   140,    91,    63,   102,   103,   104,
     141,    92,   128,    87,   137,    93,   153,   144,    85,    86,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,    94,   171,   172,   173,
     174,   175,   142,   143,    95,   124,   125,   126,    96,   181,
     182,   183,   184,   185,   186,   130,   131,   132,   133,   134,
     135,   136,    70,    74,   129,   198,    71,    75,    97,   200,
     176,   177,   178,   179,   180,   187,   188,   189,   190,   191,
     116,   199,    62,    85,   138,    63,   142,   150,   151,   192,
     193,    98,   194,   147,   195,   104,   203,   204,   205,   206,
     207,   214,   197,     0,   124,   125,   126,   208,   209,   210,
     211,   212,     0,     1,     2,     3,     4,     5,     6,     0,
       7,     0,   216,     0,     0,     0,     8,     9,    10,     0,
      11,   196,    12,    13,    62,   217,    62,    63,     0,    63,
      14,   100,   101,   102,   103,   104,     0,     0,     0,    15,
      16,    17,    18,    19,    20,    21,    22,     0,    23,    24,
      25,    26,    27,    28,    29,    30,     0,    31,    32,    33,
       0,     0,     0,    34,    35,    36,    37,    38,    39,    40,
       1,     2,     3,     4,     5,     6,     0,     7,     0,     0,
       0,     0,     0,     8,     9,    10,     0,    11,   213,    12,
      13,     0,     0,     0,     0,     0,     0,    14,     0,     0,
       0,     0,     0,     0,     0,     0,    15,    16,    17,    18,
      19,    20,    21,    22,     0,    23,    24,    25,    26,    27,
      28,    29,    30,     0,    31,    32,    33,     0,     0,     0,
      34,    35,    36,    37,    38,    39,    40,     1,     2,     3,
       4,     5,     6,     0,     7,     0,     0,     0,     0,     0,
       8,     9,    10,     0,    11,     0,    12,    13,     0,     0,
       0,     0,     0,     0,    14,     0,     0,     0,     0,     0,
       0,     0,     0,    15,    16,    17,    18,    19,    20,    21,
      22,     0,    23,    24,    25,    26,    27,    28,    29,    30,
       0,    31,    32,    33,     0,     0,     0,    34,    35,    36,
      37,    38,    39,    40,     1,     2,     3,     4,     5,     6,
       0,     7,     0,     0,     0,     0,     0,     8,     9,    10,
       0,   145,     0,    12,    13,     0,     0,     0,     0,     0,
       0,    14,     0,     0,     0,     0,     0,     0,     0,     0,
      15,    16,    17,    18,    19,    20,    21,    22,     0,    23,
      24,    25,    26,    27,    28,    29,    30,     0,    31,    32,
      33,     0,     0,     0,    34,    35,    36,    37,    38,    39,
      40,     1,     2,     0,     4,     5,     0,     0,     0,     0,
       0,     0,     0,     0,     8,     9,    10,   152,    11,     0,
      12,    13,     0,     0,     0,     0,     0,     0,    14,     0,
       0,     0,     0,     0,     0,     0,     0,    15,    76,    17,
      18,    19,    20,    21,    22,     0,    23,    24,    25,    26,
      27,    28,    29,    30,     0,    31,    32,    33,     0,     0,
       0,    34,    35,    36,    37,    38,    39,    40,     1,     2,
       0,     4,     5,     0,     0,     0,     0,     0,     0,     0,
       0,     8,     9,    10,     0,    11,     0,    12,    13,     0,
       0,     0,     0,     0,     0,    14,     0,     0,     0,     0,
       0,     4,     0,     0,    15,    76,    17,    18,    19,    20,
      21,    22,     0,    23,    24,    25,    26,    27,    28,    29,
      30,     0,    31,    32,    33,     0,     0,     0,    34,    35,
      36,    37,    38,    39,    40,    68,    17,    18,    19,    20,
      21,    22,     0,    23,    24,    25,    26,     0,     0,     0,
       0,     0,    31,    32,    33,     0,   100,   101,   102,   103,
     104,   105,   106,   107,    69,   108,   109,   110,   111,   112,
     113,   114,   115,   100,   101,   102,   103,   104,   105,   106,
     107,     0,   108,   109,   110,   111,   112,   113,   114,   115,
     149,    99,   100,   101,   102,   103,   104,   105,   106,   107,
       0,   108,   109,   110,   111,   112,   113,   215,   100,   101,
     102,   103,   104,   105,   106,   107,     0,   108,   109,   110,
     111,   112,   113,   114,   115,   148,     0,     0,   100,   101,
     102,   103,   104,   105,   106,   107,     0,   108,   109,   110,
     111,   112,   113,   114,   115,   218,   100,   101,   102,   103,
     104,   105,   106,   107,     0,   108,   109,   110,   111,   112,
     113,   114,   115,   100,   101,   102,   103,   104,   105,   106,
     107,     0,   108,   109,   110,   111,   112,   113,   114,   115
};

static const yytype_int16 yycheck[] =
{
      10,    11,    12,    13,    14,    15,    42,     0,     6,    69,
       0,     6,     7,    19,     5,     8,     9,    69,     8,     9,
       6,     7,    18,    18,     6,     7,    69,     6,     7,    69,
      66,    22,    23,    24,    25,    26,    27,    28,    29,    18,
      31,    32,    33,    34,    35,    36,    37,    38,     5,    42,
      56,    69,    42,    22,    23,    24,    25,    26,    27,    28,
      29,    60,    61,    62,    62,    56,    11,    12,    13,    14,
      15,    16,    17,    66,    69,    69,    66,    24,    25,    26,
      78,    69,    58,    69,    60,    69,    96,    69,     6,     7,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114,   115,    69,   117,   118,   119,
     120,   121,     6,     7,    69,    60,    61,    62,    18,   129,
     130,   131,   132,   133,   134,    11,    12,    13,    14,    15,
      16,    17,     8,     9,    11,   145,     8,     9,     0,   149,
      11,    12,    13,    14,    15,    11,    12,    13,    14,    15,
       5,     9,   145,     6,     6,   145,     6,    69,    69,    69,
      69,   197,    69,   199,    69,    26,   176,   177,   178,   179,
     180,   199,   145,    -1,    60,    61,    62,   187,   188,   189,
     190,   191,    -1,     3,     4,     5,     6,     7,     8,    -1,
      10,    -1,   202,    -1,    -1,    -1,    16,    17,    18,    -1,
      20,    21,    22,    23,   197,   215,   199,   197,    -1,   199,
      30,    22,    23,    24,    25,    26,    -1,    -1,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    -1,    48,    49,
      50,    51,    52,    53,    54,    55,    -1,    57,    58,    59,
      -1,    -1,    -1,    63,    64,    65,    66,    67,    68,    69,
       3,     4,     5,     6,     7,     8,    -1,    10,    -1,    -1,
      -1,    -1,    -1,    16,    17,    18,    -1,    20,    21,    22,
      23,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    -1,    48,    49,    50,    51,    52,
      53,    54,    55,    -1,    57,    58,    59,    -1,    -1,    -1,
      63,    64,    65,    66,    67,    68,    69,     3,     4,     5,
       6,     7,     8,    -1,    10,    -1,    -1,    -1,    -1,    -1,
      16,    17,    18,    -1,    20,    -1,    22,    23,    -1,    -1,
      -1,    -1,    -1,    -1,    30,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    -1,    48,    49,    50,    51,    52,    53,    54,    55,
      -1,    57,    58,    59,    -1,    -1,    -1,    63,    64,    65,
      66,    67,    68,    69,     3,     4,     5,     6,     7,     8,
      -1,    10,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      -1,    20,    -1,    22,    23,    -1,    -1,    -1,    -1,    -1,
      -1,    30,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      39,    40,    41,    42,    43,    44,    45,    46,    -1,    48,
      49,    50,    51,    52,    53,    54,    55,    -1,    57,    58,
      59,    -1,    -1,    -1,    63,    64,    65,    66,    67,    68,
      69,     3,     4,    -1,     6,     7,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    16,    17,    18,    19,    20,    -1,
      22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    30,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    41,
      42,    43,    44,    45,    46,    -1,    48,    49,    50,    51,
      52,    53,    54,    55,    -1,    57,    58,    59,    -1,    -1,
      -1,    63,    64,    65,    66,    67,    68,    69,     3,     4,
      -1,     6,     7,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    16,    17,    18,    -1,    20,    -1,    22,    23,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    -1,    -1,    -1,    -1,
      -1,     6,    -1,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    -1,    48,    49,    50,    51,    52,    53,    54,
      55,    -1,    57,    58,    59,    -1,    -1,    -1,    63,    64,
      65,    66,    67,    68,    69,    40,    41,    42,    43,    44,
      45,    46,    -1,    48,    49,    50,    51,    -1,    -1,    -1,
      -1,    -1,    57,    58,    59,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    69,    31,    32,    33,    34,    35,
      36,    37,    38,    22,    23,    24,    25,    26,    27,    28,
      29,    -1,    31,    32,    33,    34,    35,    36,    37,    38,
      56,     5,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    31,    32,    33,    34,    35,    36,    56,    22,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,    19,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    -1,    31,    32,    33,
      34,    35,    36,    37,    38,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    -1,    31,    32,    33,    34,    35,
      36,    37,    38,    22,    23,    24,    25,    26,    27,    28,
      29,    -1,    31,    32,    33,    34,    35,    36,    37,    38
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    16,    17,
      18,    20,    22,    23,    30,    39,    40,    41,    42,    43,
      44,    45,    46,    48,    49,    50,    51,    52,    53,    54,
      55,    57,    58,    59,    63,    64,    65,    66,    67,    68,
      69,    74,    76,    77,    78,    79,    80,    81,    82,    83,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    98,    99,    69,    69,    81,     5,    40,    69,
      92,    95,    98,    99,    92,    95,    40,    79,    98,    99,
      79,    79,    79,    79,    79,     6,     7,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    18,     0,    77,     5,
      22,    23,    24,    25,    26,    27,    28,    29,    31,    32,
      33,    34,    35,    36,    37,    38,     5,    11,    12,    13,
      14,    15,    16,    17,    60,    61,    62,    97,    97,    11,
      11,    12,    13,    14,    15,    16,    17,    97,     6,     7,
      69,    81,     6,     7,    69,    20,    75,    77,    19,    56,
      69,    69,    19,    79,    84,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    11,    12,    13,    14,
      15,    79,    79,    79,    79,    79,    79,    11,    12,    13,
      14,    15,    69,    69,    69,    69,    21,    76,    79,     9,
      79,    19,    56,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    21,    75,    56,    79,    79,    21
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    73,    74,    74,    75,    75,    75,    76,    76,    77,
      77,    77,    77,    77,    78,    78,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      80,    80,    80,    81,    82,    83,    83,    84,    84,    85,
      85,    86,    86,    86,    86,    86,    86,    86,    86,    86,
      86,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    88,    88,    88,    88,    88,    88,    88,    88,    89,
      89,    89,    89,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    90,    91,
      92,    92,    92,    92,    92,    92,    92,    92,    93,    93,
      93,    93,    93,    93,    93,    93,    94,    94,    94,    95,
      96,    96,    96,    96,    96,    96,    96,    96,    97,    97,
      97,    98,    98,    98,    98,    98,    98,    99,    99,    99
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     3,     2,     1,     2,     1,     2,
       2,     1,     2,     1,     3,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     3,     2,     4,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     7,
       3,     3,     2,     2,     2,     2,     3,     2,     3,     3,
       2,     2,     2,     2,     3,     2,     2,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (tree, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static unsigned
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  unsigned res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
 }

#  define YY_LOCATION_PRINT(File, Loc)          \
  yy_location_print_ (File, &(Loc))

# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, Location, tree); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, openvdb::ax::ast::Tree** tree)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (yylocationp);
  YYUSE (tree);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, openvdb::ax::ast::Tree** tree)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, tree);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, openvdb::ax::ast::Tree** tree)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                       , &(yylsp[(yyi + 1) - (yynrhs)])                       , tree);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, tree); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, openvdb::ax::ast::Tree** tree)
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (tree);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (openvdb::ax::ast::Tree** tree)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.
       'yyls': related to locations.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    /* The location stack.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls;
    YYLTYPE *yylsp;

    /* The locations where the error started and ended.  */
    YYLTYPE yyerror_range[3];

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yylsp = yyls = yylsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  yylsp[0] = yylloc;
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yyls1, yysize * sizeof (*yylsp),
                    &yystacksize);

        yyls = yyls1;
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 206 "grammar/axparser.y" /* yacc.c:1648  */
    { *tree = new Tree(); (yyval.tree) = *tree; }
#line 1750 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 3:
#line 207 "grammar/axparser.y" /* yacc.c:1648  */
    { *tree = new Tree((yyvsp[0].block)); (yyval.tree) = *tree; }
#line 1756 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 4:
#line 211 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = (yyvsp[-1].block); }
#line 1762 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 5:
#line 212 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); }
#line 1768 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 6:
#line 213 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); if((yyvsp[0].statement)) (yyval.block)->mList.emplace_back((yyvsp[0].statement)); }
#line 1774 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 7:
#line 217 "grammar/axparser.y" /* yacc.c:1648  */
    { if ((yyvsp[0].statement)) (yyvsp[-1].block)->mList.emplace_back((yyvsp[0].statement)); (yyval.block) = (yyvsp[-1].block); }
#line 1780 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 8:
#line 218 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); if ((yyvsp[0].statement)) (yyval.block)->mList.emplace_back((yyvsp[0].statement)); }
#line 1786 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 9:
#line 224 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[-1].expression); }
#line 1792 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 10:
#line 225 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[-1].statement); }
#line 1798 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 11:
#line 226 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1804 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 12:
#line 227 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new Return; }
#line 1810 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 13:
#line 228 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = nullptr; }
#line 1816 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 14:
#line 236 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new ConditionalStatement((yyvsp[-1].expression), (yyvsp[0].block), new Block()); }
#line 1822 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 15:
#line 237 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new ConditionalStatement((yyvsp[-3].expression), (yyvsp[-2].block), (yyvsp[0].block)); }
#line 1828 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 16:
#line 243 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1834 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 17:
#line 244 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1840 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 18:
#line 245 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1846 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 19:
#line 246 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1852 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 20:
#line 247 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1858 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 21:
#line 248 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1864 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 22:
#line 249 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1870 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 23:
#line 250 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1876 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 24:
#line 251 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].value); }
#line 1882 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 25:
#line 252 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].vector_unpack); }
#line 1888 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 26:
#line 253 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].value); }
#line 1894 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 27:
#line 254 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].external); }
#line 1900 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 28:
#line 255 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new LocalValue((yyvsp[0].local)); }
#line 1906 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 29:
#line 256 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AttributeValue((yyvsp[0].attribute)); }
#line 1912 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 30:
#line 263 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.vector_unpack) = new VectorUnpack(new AttributeValue((yyvsp[-1].attribute)), (yyvsp[0].index)); }
#line 1918 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 31:
#line 264 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.vector_unpack) = new VectorUnpack(new LocalValue((yyvsp[-1].local)), (yyvsp[0].index)); }
#line 1924 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 32:
#line 265 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.vector_unpack) = new VectorUnpack((yyvsp[-1].external), (yyvsp[0].index)); }
#line 1930 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 33:
#line 270 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[-1].expression); }
#line 1936 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 34:
#line 277 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Cast((yyvsp[0].expression), (yyvsp[-1].value_string)); }
#line 1942 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 35:
#line 282 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new FunctionCall((yyvsp[-3].value_string), (yyvsp[-1].expressionlist)); free((char*)(yyvsp[-3].value_string)); }
#line 1948 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 36:
#line 283 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new FunctionCall((yyvsp[-2].value_string)); free((char*)(yyvsp[-2].value_string)); }
#line 1954 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 37:
#line 288 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expressionlist) = new ExpressionList(); (yyval.expressionlist)->mList.emplace_back((yyvsp[0].expression)); }
#line 1960 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 38:
#line 289 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyvsp[-2].expressionlist)->mList.emplace_back((yyvsp[0].expression)); (yyval.expressionlist) = (yyvsp[-2].expressionlist); }
#line 1966 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 39:
#line 296 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new AssignExpression((yyvsp[-2].declare_local), (yyvsp[0].expression)); }
#line 1972 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 40:
#line 297 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[0].declare_local); }
#line 1978 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 41:
#line 306 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), (yyvsp[0].expression)); }
#line 1984 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 42:
#line 307 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::PLUS, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1990 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 43:
#line 308 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::MINUS, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1996 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 44:
#line 309 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::MULTIPLY, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 2002 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 45:
#line 310 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::DIVIDE, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 2008 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 46:
#line 311 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), (yyvsp[0].expression)); }
#line 2014 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 47:
#line 312 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::PLUS, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2020 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 48:
#line 313 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::MINUS, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2026 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 49:
#line 314 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::MULTIPLY, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2032 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 50:
#line 315 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::DIVIDE, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2038 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 51:
#line 326 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(nullptr, (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2044 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 52:
#line 327 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::PLUS), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2050 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 53:
#line 328 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MINUS), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2056 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 54:
#line 329 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2062 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 55:
#line 330 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2068 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 56:
#line 331 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(nullptr, (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2074 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 57:
#line 332 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::PLUS), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2080 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 58:
#line 333 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MINUS), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2086 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 59:
#line 334 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2092 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 60:
#line 335 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2098 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 61:
#line 344 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].attribute), new AttributeValue((yyvsp[0].attribute)->copy()), Crement::Increment, /*post*/false); }
#line 2104 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 62:
#line 345 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].attribute), new AttributeValue((yyvsp[0].attribute)->copy()), Crement::Decrement, /*post*/false); }
#line 2110 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 63:
#line 346 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].attribute), new AttributeValue((yyvsp[-1].attribute)->copy()), Crement::Increment, /*post*/true); }
#line 2116 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 64:
#line 347 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].attribute), new AttributeValue((yyvsp[-1].attribute)->copy()), Crement::Decrement, /*post*/true); }
#line 2122 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 65:
#line 348 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].local), new LocalValue((yyvsp[0].local)->copy()), Crement::Increment, /*post*/false); }
#line 2128 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 66:
#line 349 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].local), new LocalValue((yyvsp[0].local)->copy()), Crement::Decrement, /*post*/false); }
#line 2134 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 67:
#line 350 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].local), new LocalValue((yyvsp[-1].local)->copy()), Crement::Increment, /*post*/true); }
#line 2140 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 68:
#line 351 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].local), new LocalValue((yyvsp[-1].local)->copy()), Crement::Decrement, /*post*/true); }
#line 2146 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 69:
#line 356 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::PLUS, (yyvsp[0].expression)); }
#line 2152 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 70:
#line 357 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::MINUS, (yyvsp[0].expression)); }
#line 2158 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 71:
#line 358 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::BITNOT, (yyvsp[0].expression)); }
#line 2164 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 72:
#line 359 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::NOT, (yyvsp[0].expression)); }
#line 2170 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 73:
#line 365 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::PLUS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2176 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 74:
#line 366 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MINUS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2182 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 75:
#line 367 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MULTIPLY, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2188 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 76:
#line 368 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::DIVIDE, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2194 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 77:
#line 369 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MODULO, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2200 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 78:
#line 370 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITAND, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2206 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 79:
#line 371 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITOR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2212 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 80:
#line 372 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITXOR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2218 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 81:
#line 373 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::AND, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2224 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 82:
#line 374 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::OR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2230 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 83:
#line 375 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::EQUALSEQUALS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2236 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 84:
#line 376 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::NOTEQUALS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2242 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 85:
#line 377 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MORETHAN, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2248 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 86:
#line 378 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::LESSTHAN, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2254 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 87:
#line 379 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MORETHANOREQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2260 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 88:
#line 380 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::LESSTHANOREQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2266 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 89:
#line 385 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new VectorPack((yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression)); }
#line 2272 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 90:
#line 390 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2278 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 91:
#line 391 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2284 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 92:
#line 392 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<int32_t>()); free((char*)(yyvsp[0].value_string)); }
#line 2290 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 93:
#line 393 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<float>()); free((char*)(yyvsp[0].value_string)); }
#line 2296 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 94:
#line 394 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<openvdb::Vec3s>()); free((char*)(yyvsp[0].value_string)); }
#line 2302 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 95:
#line 395 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2308 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 96:
#line 396 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2314 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 97:
#line 397 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<float>(), true); free((char*)(yyvsp[0].value_string)); }
#line 2320 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 98:
#line 402 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2326 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 99:
#line 403 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2332 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 100:
#line 404 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<int32_t>()); free((char*)(yyvsp[0].value_string)); }
#line 2338 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 101:
#line 405 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<float>()); free((char*)(yyvsp[0].value_string)); }
#line 2344 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 102:
#line 406 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<openvdb::Vec3s>()); free((char*)(yyvsp[0].value_string)); }
#line 2350 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 103:
#line 407 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2356 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 104:
#line 408 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2362 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 105:
#line 409 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.external) = new ExternalVariable((yyvsp[0].value_string), openvdb::typeNameAsString<float>()); free((char*)(yyvsp[0].value_string)); }
#line 2368 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 106:
#line 414 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), (yyvsp[-1].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2374 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 107:
#line 415 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), (yyvsp[-1].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2380 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 108:
#line 416 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2386 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 109:
#line 423 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.local) = new Local((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2392 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 110:
#line 430 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int16_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2398 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 111:
#line 431 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int32_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2404 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 112:
#line 432 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int64_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2410 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 113:
#line 433 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<float>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2416 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 114:
#line 434 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<double>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2422 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 115:
#line 435 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<std::string>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2428 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 116:
#line 436 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<bool>(true); }
#line 2434 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 117:
#line 437 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<bool>(false); }
#line 2440 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 118:
#line 442 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 0; }
#line 2446 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 119:
#line 443 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 1; }
#line 2452 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 120:
#line 444 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 2; }
#line 2458 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 121:
#line 450 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<bool>(); }
#line 2464 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 122:
#line 451 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int16_t>(); }
#line 2470 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 123:
#line 452 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int32_t>(); }
#line 2476 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 124:
#line 453 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int64_t>(); }
#line 2482 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 125:
#line 454 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<float>(); }
#line 2488 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 126:
#line 455 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<double>(); }
#line 2494 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 127:
#line 461 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3i>(); }
#line 2500 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 128:
#line 462 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3s>(); }
#line 2506 "grammar/axparser.cc" /* yacc.c:1648  */
    break;

  case 129:
#line 463 "grammar/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3d>(); }
#line 2512 "grammar/axparser.cc" /* yacc.c:1648  */
    break;


#line 2516 "grammar/axparser.cc" /* yacc.c:1648  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (tree, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (tree, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }

  yyerror_range[1] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, tree);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, yylsp, tree);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the lookahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, yyerror_range, 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (tree, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, tree);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, yylsp, tree);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 465 "grammar/axparser.y" /* yacc.c:1907  */


// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
