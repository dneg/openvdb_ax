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

/// @file ast/Parse.h
///
/// @authors Nick Avramoussis, Richard Jones
///
/// @brief Parsing methods for creating abstract syntax trees out of AX code
///

#ifndef OPENVDB_AX_PARSE_HAS_BEEN_INCLUDED
#define OPENVDB_AX_PARSE_HAS_BEEN_INCLUDED

#include "AST.h"
#include "../compiler/Logger.h"

#include <openvdb/version.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

/// @brief  Construct an abstract syntax tree from a code snippet. If the code is
///         not well formed, as defined by the AX grammar, this will simply return
///         nullptr, with the logger collecting the errors.
/// @note   The returned AST is const as the logger uses this to determine line
///         and column numbers of errors/warnings in later stages. If you need to
///         modify the tree, take a copy.
///
/// @return A shared pointer to a valid const AST, or nullptr if errored.
///
/// @param code    The code to parse
/// @param logger  The logger to collect syntax errors
///
openvdb::ax::ast::Tree::ConstPtr parse(const char* code, ax::Logger& logger);

/// @brief  Construct an abstract syntax tree from a code snippet.
///         A runtime exception will be thrown with the first syntax error.
///
/// @return A shared pointer to a valid AST.
///
/// @param code The code to parse
///
openvdb::ax::ast::Tree::Ptr parse(const char* code);

} // namespace ast
} // namespace ax

} // namespace OPENVDB_VERSION_NAME
} // namespace openvdb

#endif // OPENVDB_AX_AST_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
