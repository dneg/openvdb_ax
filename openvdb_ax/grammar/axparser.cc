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
#line 31 "ax/axparser.y" /* yacc.c:339  */

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

#line 145 "ax/axparser.cc" /* yacc.c:339  */

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
#ifndef YY_YY_AX_AXPARSER_H_INCLUDED
# define YY_YY_AX_AXPARSER_H_INCLUDED
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
#line 114 "ax/axparser.y" /* yacc.c:355  */

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

#line 271 "ax/axparser.cc" /* yacc.c:355  */
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

#endif /* !YY_YY_AX_AXPARSER_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 302 "ax/axparser.cc" /* yacc.c:358  */

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
#define YYFINAL  83
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   726

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  26
/* YYNRULES -- Number of rules.  */
#define YYNRULES  119
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  199

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   322

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
      65,    66,    67
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   204,   204,   205,   209,   210,   211,   215,   216,   222,
     223,   224,   225,   226,   234,   235,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   260,
     261,   266,   273,   278,   279,   284,   285,   292,   293,   302,
     303,   304,   305,   306,   307,   308,   309,   310,   311,   322,
     323,   324,   325,   326,   327,   328,   329,   330,   331,   340,
     341,   342,   343,   344,   345,   346,   347,   352,   353,   354,
     355,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   381,   386,   387,
     388,   389,   390,   391,   392,   393,   398,   399,   400,   407,
     414,   415,   416,   417,   418,   419,   420,   421,   426,   427,
     428,   434,   435,   436,   437,   438,   439,   445,   446,   447
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TRUE", "FALSE", "SEMICOLON", "AT", "IF",
  "ELSE", "RETURN", "EQUALS", "PLUSEQUALS", "MINUSEQUALS",
  "MULTIPLYEQUALS", "DIVIDEEQUALS", "PLUSPLUS", "MINUSMINUS", "LPARENS",
  "RPARENS", "LCURLY", "RCURLY", "PLUS", "MINUS", "MULTIPLY", "DIVIDE",
  "MODULO", "BITAND", "BITOR", "BITXOR", "BITNOT", "EQUALSEQUALS",
  "NOTEQUALS", "MORETHAN", "LESSTHAN", "MORETHANOREQUAL",
  "LESSTHANOREQUAL", "AND", "OR", "NOT", "STRING", "DOUBLE", "FLOAT",
  "LONG", "INT", "SHORT", "BOOL", "VOID", "F_AT", "I_AT", "V_AT", "S_AT",
  "COMMA", "VEC3I", "VEC3F", "VEC3D", "DOT_X", "DOT_Y", "DOT_Z", "L_SHORT",
  "L_INT", "L_LONG", "L_FLOAT", "L_DOUBLE", "L_STRING", "IDENTIFIER",
  "LPAREN", "RPAREN", "LOWER_THAN_ELSE", "$accept", "statements", "block",
  "body", "statement", "conditional_statement", "expression",
  "vector_element", "expression_expand", "cast_expression",
  "function_call_expression", "arguments", "declare_assignment",
  "assign_expression", "assign_component_expression", "crement",
  "unary_expression", "binary_expression", "vector_literal", "attribute",
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
     315,   316,   317,   318,   319,   320,   321,   322
};
# endif

#define YYPACT_NINF -58

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-58)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     299,   -58,   -58,   -58,   -48,    19,    14,   511,   511,   485,
     485,   485,   485,   485,   485,    11,   -58,   -58,   -58,   -58,
     -58,   -58,    -8,    -7,     7,    29,   -58,   -58,   -58,   -58,
     -58,   -58,   -58,   -58,   -58,    25,    64,   299,   -58,   -58,
     593,   -58,   -58,   -58,   -58,    61,   -58,   -58,   -58,   -58,
     -58,   -58,   114,    58,   556,   -58,     3,    12,   -58,   361,
     -58,    68,   -58,   -58,   -58,    98,   110,   -58,   -58,   654,
      23,   610,    21,    21,   -58,     0,    53,   -58,   -58,   -58,
     -58,   -58,   423,   -58,   -58,   -58,   485,   485,   485,   485,
     485,   485,   485,   485,   485,   485,   485,   485,   485,   485,
     485,   485,   -58,   485,   485,   485,   485,   485,   -58,   -58,
     -58,   -58,   -58,    49,   485,   485,   485,   485,   485,   485,
     -58,   -58,    89,    54,   -58,   -58,    56,   -58,   175,    62,
     -58,   -58,   485,   -58,   -58,   689,   -10,    21,    21,    96,
      96,   -58,    90,    90,    90,     0,     0,    27,    27,    27,
      27,     0,     0,   689,   689,   689,   689,   689,   485,   485,
     485,   485,   485,   689,   689,   689,   689,   689,   689,   485,
     485,   485,   485,   485,   -58,   -58,   -58,   237,   557,   361,
     632,   -58,   485,   689,   689,   689,   689,   689,   689,   689,
     689,   689,   689,   -58,   -58,   485,   689,   672,   -58
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,   106,   107,    13,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   116,   115,   114,   113,
     112,   111,     0,     0,     0,     0,   117,   118,   119,   100,
     101,   102,   103,   104,   105,    99,     0,     3,     8,    11,
       0,    25,    16,    23,    17,     0,    20,    21,    22,    19,
      18,    24,    28,    38,    27,    26,     0,     0,    95,     0,
      12,     0,    99,    59,    63,     0,     0,    60,    64,     0,
       0,     0,    67,    68,    69,    70,     0,    98,    91,    90,
      92,    93,     0,     1,     7,     9,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,     0,     0,     0,     0,     0,    61,    62,
     108,   109,   110,    29,     0,     0,     0,     0,     0,     0,
      65,    66,    30,     0,    96,    32,     0,    97,     0,    14,
       6,    31,     0,    94,    34,    35,     0,    71,    72,    73,
      74,    75,    76,    77,    78,    81,    82,    83,    84,    85,
      86,    79,    80,    39,    40,    41,    42,    43,     0,     0,
       0,     0,     0,    37,    44,    45,    46,    47,    48,     0,
       0,     0,     0,     0,    88,    89,     5,     0,     0,     0,
       0,    33,     0,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,     4,    15,     0,    36,     0,    87
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -58,   -58,   -57,     5,   -22,   -58,    -9,   -58,     2,   -58,
     -58,   -58,   -58,   -58,   -58,   -58,   -58,   -58,   -58,     4,
     -58,    31,   -58,    77,     6,    10
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    36,   129,    37,    38,    39,    40,    41,    42,    43,
      44,   136,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,   113,    70,    66
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      69,    71,    72,    73,    74,    75,    56,    59,   181,   123,
      57,    63,    67,    65,    65,    84,    58,    76,   126,    60,
       9,    86,    87,    88,    89,    90,    91,    92,    93,   123,
      94,    95,    96,    97,    98,    99,     9,   130,    64,    68,
       9,   182,    82,    56,    88,    89,    90,    57,    86,    87,
      88,    89,    90,    91,    92,    93,    78,    79,   125,   158,
     159,   160,   161,   162,    83,    56,   102,   124,   114,    57,
     179,    80,   125,   135,    76,    77,   127,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,    81,   153,   154,   155,   156,   157,   169,
     170,   171,   172,   173,   123,   163,   164,   165,   166,   167,
     168,    86,    87,    88,    89,    90,   126,   133,   174,   178,
     175,    90,   194,   180,   103,   104,   105,   106,   107,   108,
     109,   122,     0,   177,    56,     0,     0,     0,    57,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   183,
     184,   185,   186,   187,     0,    84,     0,   130,     0,     0,
     188,   189,   190,   191,   192,     0,     0,     0,     0,   110,
     111,   112,     0,   196,     0,     0,     0,     0,     1,     2,
       3,     4,     5,    56,     6,    56,   197,    57,     0,    57,
       7,     8,     9,     0,    10,   176,    11,    12,     0,     0,
       0,     0,     0,     0,    13,     0,     0,     0,     0,     0,
       0,     0,     0,    14,    15,    16,    17,    18,    19,    20,
      21,     0,    22,    23,    24,    25,     0,    26,    27,    28,
       0,     0,     0,    29,    30,    31,    32,    33,    34,    35,
       1,     2,     3,     4,     5,     0,     6,     0,     0,     0,
       0,     0,     7,     8,     9,     0,    10,   193,    11,    12,
       0,     0,     0,     0,     0,     0,    13,     0,     0,     0,
       0,     0,     0,     0,     0,    14,    15,    16,    17,    18,
      19,    20,    21,     0,    22,    23,    24,    25,     0,    26,
      27,    28,     0,     0,     0,    29,    30,    31,    32,    33,
      34,    35,     1,     2,     3,     4,     5,     0,     6,     0,
       0,     0,     0,     0,     7,     8,     9,     0,    10,     0,
      11,    12,     0,     0,     0,     0,     0,     0,    13,     0,
       0,     0,     0,     0,     0,     0,     0,    14,    15,    16,
      17,    18,    19,    20,    21,     0,    22,    23,    24,    25,
       0,    26,    27,    28,     0,     0,     0,    29,    30,    31,
      32,    33,    34,    35,     1,     2,     3,     4,     5,     0,
       6,     0,     0,     0,     0,     0,     7,     8,     9,     0,
     128,     0,    11,    12,     0,     0,     0,     0,     0,     0,
      13,     0,     0,     0,     0,     0,     0,     0,     0,    14,
      15,    16,    17,    18,    19,    20,    21,     0,    22,    23,
      24,    25,     0,    26,    27,    28,     0,     0,     0,    29,
      30,    31,    32,    33,    34,    35,     1,     2,     0,     4,
       0,     0,     0,     0,     0,     0,     0,     0,     7,     8,
       9,   134,    10,     0,    11,    12,     0,     0,     0,     0,
       0,     0,    13,     0,     0,     0,     0,     0,     0,     0,
       0,    14,    61,    16,    17,    18,    19,    20,    21,     0,
      22,    23,    24,    25,     0,    26,    27,    28,     0,     0,
       0,    29,    30,    31,    32,    33,    34,    35,     1,     2,
       0,     4,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9,     0,    10,     0,    11,    12,     0,     0,
       0,     0,     0,     0,    13,     0,     0,     4,     0,     0,
       0,     0,     0,    14,    61,    16,    17,    18,    19,    20,
      21,     0,    22,    23,    24,    25,     0,    26,    27,    28,
       0,     0,     0,    29,    30,    31,    32,    33,    34,    35,
      61,    16,    17,    18,    19,    20,    21,     0,    22,    23,
      24,    25,    85,    26,    27,    28,   115,   116,   117,   118,
     119,   120,   121,     0,     0,    62,     0,     0,    86,    87,
      88,    89,    90,    91,    92,    93,     0,    94,    95,    96,
      97,    98,    99,   100,   101,     0,     0,     0,    85,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   132,     0,
       0,   110,   111,   112,    86,    87,    88,    89,    90,    91,
      92,    93,     0,    94,    95,    96,    97,    98,    99,   100,
     101,    86,    87,    88,    89,    90,    91,    92,    93,     0,
      94,    95,    96,    97,    98,    99,   100,   101,     0,     0,
       0,     0,     0,    86,    87,    88,    89,    90,    91,    92,
      93,   132,    94,    95,    96,    97,    98,    99,   100,   101,
       0,     0,   131,     0,     0,    86,    87,    88,    89,    90,
      91,    92,    93,   195,    94,    95,    96,    97,    98,    99,
     100,   101,   198,    86,    87,    88,    89,    90,    91,    92,
      93,     0,    94,    95,    96,    97,    98,    99,   100,   101,
      86,    87,    88,    89,    90,    91,    92,    93,     0,    94,
      95,    96,    97,    98,    99,   100,   101
};

static const yytype_int16 yycheck[] =
{
       9,    10,    11,    12,    13,    14,     0,     5,    18,     6,
       0,     7,     8,     7,     8,    37,    64,     6,     6,     5,
      17,    21,    22,    23,    24,    25,    26,    27,    28,     6,
      30,    31,    32,    33,    34,    35,    17,    59,     7,     8,
      17,    51,    17,    37,    23,    24,    25,    37,    21,    22,
      23,    24,    25,    26,    27,    28,    64,    64,    56,    10,
      11,    12,    13,    14,     0,    59,     5,    64,    10,    59,
       8,    64,    70,    82,     6,    64,    64,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,    64,   103,   104,   105,   106,   107,    10,
      11,    12,    13,    14,     6,   114,   115,   116,   117,   118,
     119,    21,    22,    23,    24,    25,     6,    64,    64,   128,
      64,    25,   179,   132,    10,    11,    12,    13,    14,    15,
      16,    54,    -1,   128,   128,    -1,    -1,    -1,   128,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   158,
     159,   160,   161,   162,    -1,   177,    -1,   179,    -1,    -1,
     169,   170,   171,   172,   173,    -1,    -1,    -1,    -1,    55,
      56,    57,    -1,   182,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,   177,     9,   179,   195,   177,    -1,   179,
      15,    16,    17,    -1,    19,    20,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    49,    50,    -1,    52,    53,    54,
      -1,    -1,    -1,    58,    59,    60,    61,    62,    63,    64,
       3,     4,     5,     6,     7,    -1,     9,    -1,    -1,    -1,
      -1,    -1,    15,    16,    17,    -1,    19,    20,    21,    22,
      -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    38,    39,    40,    41,    42,
      43,    44,    45,    -1,    47,    48,    49,    50,    -1,    52,
      53,    54,    -1,    -1,    -1,    58,    59,    60,    61,    62,
      63,    64,     3,     4,     5,     6,     7,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    15,    16,    17,    -1,    19,    -1,
      21,    22,    -1,    -1,    -1,    -1,    -1,    -1,    29,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,    39,    40,
      41,    42,    43,    44,    45,    -1,    47,    48,    49,    50,
      -1,    52,    53,    54,    -1,    -1,    -1,    58,    59,    60,
      61,    62,    63,    64,     3,     4,     5,     6,     7,    -1,
       9,    -1,    -1,    -1,    -1,    -1,    15,    16,    17,    -1,
      19,    -1,    21,    22,    -1,    -1,    -1,    -1,    -1,    -1,
      29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    50,    -1,    52,    53,    54,    -1,    -1,    -1,    58,
      59,    60,    61,    62,    63,    64,     3,     4,    -1,     6,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    15,    16,
      17,    18,    19,    -1,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    39,    40,    41,    42,    43,    44,    45,    -1,
      47,    48,    49,    50,    -1,    52,    53,    54,    -1,    -1,
      -1,    58,    59,    60,    61,    62,    63,    64,     3,     4,
      -1,     6,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      15,    16,    17,    -1,    19,    -1,    21,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    29,    -1,    -1,     6,    -1,    -1,
      -1,    -1,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    -1,    47,    48,    49,    50,    -1,    52,    53,    54,
      -1,    -1,    -1,    58,    59,    60,    61,    62,    63,    64,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    50,     5,    52,    53,    54,    10,    11,    12,    13,
      14,    15,    16,    -1,    -1,    64,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    -1,    30,    31,    32,
      33,    34,    35,    36,    37,    -1,    -1,    -1,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,    -1,
      -1,    55,    56,    57,    21,    22,    23,    24,    25,    26,
      27,    28,    -1,    30,    31,    32,    33,    34,    35,    36,
      37,    21,    22,    23,    24,    25,    26,    27,    28,    -1,
      30,    31,    32,    33,    34,    35,    36,    37,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    51,    30,    31,    32,    33,    34,    35,    36,    37,
      -1,    -1,    18,    -1,    -1,    21,    22,    23,    24,    25,
      26,    27,    28,    51,    30,    31,    32,    33,    34,    35,
      36,    37,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    -1,    30,    31,    32,    33,    34,    35,    36,    37,
      21,    22,    23,    24,    25,    26,    27,    28,    -1,    30,
      31,    32,    33,    34,    35,    36,    37
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     9,    15,    16,    17,
      19,    21,    22,    29,    38,    39,    40,    41,    42,    43,
      44,    45,    47,    48,    49,    50,    52,    53,    54,    58,
      59,    60,    61,    62,    63,    64,    69,    71,    72,    73,
      74,    75,    76,    77,    78,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    92,    93,    64,    76,
       5,    39,    64,    87,    89,    92,    93,    87,    89,    74,
      92,    74,    74,    74,    74,    74,     6,    64,    64,    64,
      64,    64,    17,     0,    72,     5,    21,    22,    23,    24,
      25,    26,    27,    28,    30,    31,    32,    33,    34,    35,
      36,    37,     5,    10,    11,    12,    13,    14,    15,    16,
      55,    56,    57,    91,    10,    10,    11,    12,    13,    14,
      15,    16,    91,     6,    64,    76,     6,    64,    19,    70,
      72,    18,    51,    64,    18,    74,    79,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    10,    11,
      12,    13,    14,    74,    74,    74,    74,    74,    74,    10,
      11,    12,    13,    14,    64,    64,    20,    71,    74,     8,
      74,    18,    51,    74,    74,    74,    74,    74,    74,    74,
      74,    74,    74,    20,    70,    51,    74,    74,    20
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    68,    69,    69,    70,    70,    70,    71,    71,    72,
      72,    72,    72,    72,    73,    73,    74,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    75,
      75,    76,    77,    78,    78,    79,    79,    80,    80,    81,
      81,    81,    81,    81,    81,    81,    81,    81,    81,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    83,
      83,    83,    83,    83,    83,    83,    83,    84,    84,    84,
      84,    85,    85,    85,    85,    85,    85,    85,    85,    85,
      85,    85,    85,    85,    85,    85,    85,    86,    87,    87,
      87,    87,    87,    87,    87,    87,    88,    88,    88,    89,
      90,    90,    90,    90,    90,    90,    90,    90,    91,    91,
      91,    92,    92,    92,    92,    92,    92,    93,    93,    93
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     3,     2,     1,     2,     1,     2,
       2,     1,     2,     1,     3,     5,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     3,     2,     4,     3,     1,     3,     3,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     4,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     7,     3,     3,
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
#line 204 "ax/axparser.y" /* yacc.c:1648  */
    { *tree = new Tree(); (yyval.tree) = *tree; }
#line 1733 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 3:
#line 205 "ax/axparser.y" /* yacc.c:1648  */
    { *tree = new Tree((yyvsp[0].block)); (yyval.tree) = *tree; }
#line 1739 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 4:
#line 209 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = (yyvsp[-1].block); }
#line 1745 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 5:
#line 210 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); }
#line 1751 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 6:
#line 211 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); if((yyvsp[0].statement)) (yyval.block)->mList.emplace_back((yyvsp[0].statement)); }
#line 1757 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 7:
#line 215 "ax/axparser.y" /* yacc.c:1648  */
    { if ((yyvsp[0].statement)) (yyvsp[-1].block)->mList.emplace_back((yyvsp[0].statement)); (yyval.block) = (yyvsp[-1].block); }
#line 1763 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 8:
#line 216 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.block) = new Block(); if ((yyvsp[0].statement)) (yyval.block)->mList.emplace_back((yyvsp[0].statement)); }
#line 1769 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 9:
#line 222 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[-1].expression); }
#line 1775 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 10:
#line 223 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[-1].statement); }
#line 1781 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 11:
#line 224 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[0].statement); }
#line 1787 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 12:
#line 225 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new Return; }
#line 1793 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 13:
#line 226 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = nullptr; }
#line 1799 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 14:
#line 234 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new ConditionalStatement((yyvsp[-1].expression), (yyvsp[0].block), new Block()); }
#line 1805 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 15:
#line 235 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new ConditionalStatement((yyvsp[-3].expression), (yyvsp[-2].block), (yyvsp[0].block)); }
#line 1811 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 16:
#line 241 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1817 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 17:
#line 242 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1823 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 18:
#line 243 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1829 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 19:
#line 244 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1835 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 20:
#line 245 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1841 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 21:
#line 246 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1847 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 22:
#line 247 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1853 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 23:
#line 248 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].expression); }
#line 1859 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 24:
#line 249 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].value); }
#line 1865 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 25:
#line 250 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].vector_unpack); }
#line 1871 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 26:
#line 251 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[0].value); }
#line 1877 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 27:
#line 252 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new LocalValue((yyvsp[0].local)); }
#line 1883 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 28:
#line 253 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AttributeValue((yyvsp[0].attribute)); }
#line 1889 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 29:
#line 260 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.vector_unpack) = new VectorUnpack(new AttributeValue((yyvsp[-1].attribute)), (yyvsp[0].index)); }
#line 1895 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 30:
#line 261 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.vector_unpack) = new VectorUnpack(new LocalValue((yyvsp[-1].local)), (yyvsp[0].index)); }
#line 1901 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 31:
#line 266 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = (yyvsp[-1].expression); }
#line 1907 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 32:
#line 273 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Cast((yyvsp[0].expression), (yyvsp[-1].value_string)); }
#line 1913 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 33:
#line 278 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new FunctionCall((yyvsp[-3].value_string), (yyvsp[-1].expressionlist)); free((char*)(yyvsp[-3].value_string)); }
#line 1919 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 34:
#line 279 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new FunctionCall((yyvsp[-2].value_string)); free((char*)(yyvsp[-2].value_string)); }
#line 1925 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 35:
#line 284 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expressionlist) = new ExpressionList(); (yyval.expressionlist)->mList.emplace_back((yyvsp[0].expression)); }
#line 1931 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 36:
#line 285 "ax/axparser.y" /* yacc.c:1648  */
    { (yyvsp[-2].expressionlist)->mList.emplace_back((yyvsp[0].expression)); (yyval.expressionlist) = (yyvsp[-2].expressionlist); }
#line 1937 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 37:
#line 292 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = new AssignExpression((yyvsp[-2].declare_local), (yyvsp[0].expression)); }
#line 1943 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 38:
#line 293 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.statement) = (yyvsp[0].declare_local); }
#line 1949 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 39:
#line 302 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), (yyvsp[0].expression)); }
#line 1955 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 40:
#line 303 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::PLUS, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1961 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 41:
#line 304 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::MINUS, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1967 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 42:
#line 305 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::MULTIPLY, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1973 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 43:
#line 306 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].attribute), new BinaryOperator(tokens::DIVIDE, new AttributeValue((yyvsp[-2].attribute)->copy()), (yyvsp[0].expression))); }
#line 1979 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 44:
#line 307 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), (yyvsp[0].expression)); }
#line 1985 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 45:
#line 308 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::PLUS, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 1991 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 46:
#line 309 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::MINUS, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 1997 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 47:
#line 310 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::MULTIPLY, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2003 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 48:
#line 311 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new AssignExpression((yyvsp[-2].local), new BinaryOperator(tokens::DIVIDE, new LocalValue((yyvsp[-2].local)->copy()), (yyvsp[0].expression))); }
#line 2009 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 49:
#line 322 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(nullptr, (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2015 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 50:
#line 323 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::PLUS), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2021 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 51:
#line 324 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MINUS), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2027 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 52:
#line 325 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2033 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 53:
#line 326 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildAttributeComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), (yyvsp[-3].attribute), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2039 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 54:
#line 327 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(nullptr, (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2045 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 55:
#line 328 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::PLUS), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2051 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 56:
#line 329 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MINUS), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2057 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 57:
#line 330 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::MULTIPLY), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2063 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 58:
#line 331 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = buildLocalComponentExpression(new tokens::OperatorToken(tokens::DIVIDE), (yyvsp[-3].local), (yyvsp[-2].index), (yyvsp[0].expression)); }
#line 2069 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 59:
#line 340 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].attribute), new AttributeValue((yyvsp[0].attribute)->copy()), Crement::Increment, /*post*/false); }
#line 2075 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 60:
#line 341 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].attribute), new AttributeValue((yyvsp[0].attribute)->copy()), Crement::Decrement, /*post*/false); }
#line 2081 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 61:
#line 342 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].attribute), new AttributeValue((yyvsp[-1].attribute)->copy()), Crement::Increment, /*post*/true); }
#line 2087 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 62:
#line 343 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].attribute), new AttributeValue((yyvsp[-1].attribute)->copy()), Crement::Decrement, /*post*/true); }
#line 2093 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 63:
#line 344 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].local), new LocalValue((yyvsp[0].local)->copy()), Crement::Increment, /*post*/false); }
#line 2099 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 64:
#line 345 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[0].local), new LocalValue((yyvsp[0].local)->copy()), Crement::Decrement, /*post*/false); }
#line 2105 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 65:
#line 346 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].local), new LocalValue((yyvsp[-1].local)->copy()), Crement::Increment, /*post*/true); }
#line 2111 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 66:
#line 347 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new Crement((yyvsp[-1].local), new LocalValue((yyvsp[-1].local)->copy()), Crement::Decrement, /*post*/true); }
#line 2117 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 67:
#line 352 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::PLUS, (yyvsp[0].expression)); }
#line 2123 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 68:
#line 353 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::MINUS, (yyvsp[0].expression)); }
#line 2129 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 69:
#line 354 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::BITNOT, (yyvsp[0].expression)); }
#line 2135 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 70:
#line 355 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new UnaryOperator(tokens::NOT, (yyvsp[0].expression)); }
#line 2141 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 71:
#line 361 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::PLUS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2147 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 72:
#line 362 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MINUS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2153 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 73:
#line 363 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MULTIPLY, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2159 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 74:
#line 364 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::DIVIDE, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2165 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 75:
#line 365 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MODULO, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2171 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 76:
#line 366 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITAND, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2177 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 77:
#line 367 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITOR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2183 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 78:
#line 368 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::BITXOR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2189 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 79:
#line 369 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::AND, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2195 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 80:
#line 370 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::OR, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2201 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 81:
#line 371 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::EQUALSEQUALS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2207 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 82:
#line 372 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::NOTEQUALS, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2213 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 83:
#line 373 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MORETHAN, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2219 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 84:
#line 374 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::LESSTHAN, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2225 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 85:
#line 375 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::MORETHANOREQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2231 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 86:
#line 376 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.expression) = new BinaryOperator(tokens::LESSTHANOREQUAL, (yyvsp[-2].expression), (yyvsp[0].expression)); }
#line 2237 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 87:
#line 381 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new VectorPack((yyvsp[-5].expression), (yyvsp[-3].expression), (yyvsp[-1].expression)); }
#line 2243 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 88:
#line 386 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2249 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 89:
#line 387 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), (yyvsp[-2].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2255 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 90:
#line 388 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<int32_t>()); free((char*)(yyvsp[0].value_string)); }
#line 2261 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 91:
#line 389 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<float>()); free((char*)(yyvsp[0].value_string)); }
#line 2267 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 92:
#line 390 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<openvdb::Vec3s>()); free((char*)(yyvsp[0].value_string)); }
#line 2273 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 93:
#line 391 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2279 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 94:
#line 392 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2285 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 95:
#line 393 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.attribute) = new Attribute((yyvsp[0].value_string), openvdb::typeNameAsString<float>(), true); free((char*)(yyvsp[0].value_string)); }
#line 2291 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 96:
#line 398 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), (yyvsp[-1].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2297 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 97:
#line 399 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), (yyvsp[-1].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2303 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 98:
#line 400 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.declare_local) = new DeclareLocal((yyvsp[0].value_string), openvdb::typeNameAsString<std::string>()); free((char*)(yyvsp[0].value_string)); }
#line 2309 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 99:
#line 407 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.local) = new Local((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2315 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 100:
#line 414 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int16_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2321 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 101:
#line 415 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int32_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2327 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 102:
#line 416 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<int64_t>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2333 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 103:
#line 417 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<float>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2339 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 104:
#line 418 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<double>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2345 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 105:
#line 419 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<std::string>((yyvsp[0].value_string)); free((char*)(yyvsp[0].value_string)); }
#line 2351 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 106:
#line 420 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<bool>(true); }
#line 2357 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 107:
#line 421 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value) = new Value<bool>(false); }
#line 2363 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 108:
#line 426 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 0; }
#line 2369 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 109:
#line 427 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 1; }
#line 2375 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 110:
#line 428 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.index) = 2; }
#line 2381 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 111:
#line 434 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<bool>(); }
#line 2387 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 112:
#line 435 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int16_t>(); }
#line 2393 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 113:
#line 436 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int32_t>(); }
#line 2399 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 114:
#line 437 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<int64_t>(); }
#line 2405 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 115:
#line 438 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<float>(); }
#line 2411 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 116:
#line 439 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<double>(); }
#line 2417 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 117:
#line 445 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3i>(); }
#line 2423 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 118:
#line 446 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3s>(); }
#line 2429 "ax/axparser.cc" /* yacc.c:1648  */
    break;

  case 119:
#line 447 "ax/axparser.y" /* yacc.c:1648  */
    { (yyval.value_string) = openvdb::typeNameAsString<openvdb::Vec3d>(); }
#line 2435 "ax/axparser.cc" /* yacc.c:1648  */
    break;


#line 2439 "ax/axparser.cc" /* yacc.c:1648  */
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
#line 449 "ax/axparser.y" /* yacc.c:1907  */


// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
