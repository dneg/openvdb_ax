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

/// @file ast/Tokens.h
///
/// @authors Nick Avramoussis
///
/// @brief  Various function and operator tokens used throughout the
///         AST and code generation
///

#ifndef OPENVDB_AX_AST_TOKENS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_AST_TOKENS_HAS_BEEN_INCLUDED

#include <openvdb/util/Name.h>
#include <openvdb_ax/version.h>

#include <openvdb_ax/Exceptions.h>

#include <stdexcept>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

namespace tokens {

enum OperatorToken
{
    ////////////////////////////////////////////////////////////////
    ///
    ///  ARITHMETIC
    ///
    ////////////////////////////////////////////////////////////////
    PLUS = 0,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ////////////////////////////////////////////////////////////////
    ///
    ///  LOGICAL
    ///
    ////////////////////////////////////////////////////////////////
    AND,
    OR,
    NOT,
    ////////////////////////////////////////////////////////////////
    ///
    ///  RELATIONAL
    ///
    ////////////////////////////////////////////////////////////////
    EQUALSEQUALS,
    NOTEQUALS,
    MORETHAN,
    LESSTHAN,
    MORETHANOREQUAL,
    LESSTHANOREQUAL,
    ////////////////////////////////////////////////////////////////
    ///
    ///  BITWISE
    ///
    ////////////////////////////////////////////////////////////////
    BITAND,
    BITOR,
    BITXOR,
    BITNOT
};

enum OperatorType
{
    ARITHMETIC = 0,
    LOGICAL,
    RELATIONAL,
    BITWISE,
    UNKNOWN_OPERATOR
};

inline OperatorType operatorType(const OperatorToken token)
{
    const size_t idx = static_cast<size_t>(token);
    if (idx <= static_cast<size_t>(MODULO))          return ARITHMETIC;
    if (idx <= static_cast<size_t>(NOT))             return LOGICAL;
    if (idx <= static_cast<size_t>(LESSTHANOREQUAL)) return RELATIONAL;
    if (idx <= static_cast<size_t>(BITNOT))          return BITWISE;
    return UNKNOWN_OPERATOR;
};

inline OperatorToken operatorTokenFromName(const Name& name)
{
    if (name == "+")   return  PLUS;
    if (name == "-")   return  MINUS;
    if (name == "*")   return  MULTIPLY;
    if (name == "/")   return  DIVIDE;
    if (name == "%")   return  MODULO;
    if (name == "&&")  return  AND;
    if (name == "||")  return  OR;
    if (name == "!")   return  NOT;
    if (name == "==")  return  EQUALSEQUALS;
    if (name == "!=")  return  NOTEQUALS;
    if (name == ">")   return  MORETHAN;
    if (name == "<")   return  LESSTHAN;
    if (name == ">=")  return  MORETHANOREQUAL;
    if (name == "<=")  return  LESSTHANOREQUAL;
    if (name == "&")   return  BITAND;
    if (name == "|")   return  BITOR;
    if (name == "^")   return  BITXOR;
    if (name == "~")   return  BITNOT;
    OPENVDB_THROW(AXSyntaxError, "Unsupported op \"" + name + "\"");
}

inline Name operatorNameFromToken(const OperatorToken token)
{
    switch (token) {
        case PLUS              : return "+";
        case MINUS             : return "-";
        case MULTIPLY          : return "*";
        case DIVIDE            : return "/";
        case MODULO            : return "%";
        case AND               : return "&&";
        case OR                : return "||";
        case NOT               : return "!";
        case EQUALSEQUALS      : return "==";
        case NOTEQUALS         : return "!=";
        case MORETHAN          : return ">";
        case LESSTHAN          : return "<";
        case MORETHANOREQUAL   : return ">=";
        case LESSTHANOREQUAL   : return "<=";
        case BITAND            : return "&";
        case BITOR             : return "|";
        case BITXOR            : return "^";
        case BITNOT            : return "~";
        default                :
            OPENVDB_THROW(AXSyntaxError, "Unsupported op");
    }
}

}

}
}
}
}

#endif // OPENVDB_AX_AST_TOKENS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
