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

#ifndef OPENVDB_AX_COMPILER_AST_SCANNERS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPILER_AST_SCANNERS_HAS_BEEN_INCLUDED

#include "AST.h"

#include <memory>
#include <string>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

/// @brief  Returns whether or not a given syntax tree reads from or writes to a given attribute
///
/// @param tree  The AST to analyze
/// @param name  the name of the attribute to search for
///
inline bool usesAttribute(const ast::Tree& tree, const std::string& name);

/// @brief  Returns whether or not a provided attribute is being written to in an abstract
///         syntax tree
///
/// @param tree  The AST to analyze
/// @param name  the name of the attribute to search for
///
inline bool writesToAttribute(const ast::Tree& tree, const std::string& name);

/// @brief  Returns whether or not a function is being called in an abstract syntax tree
///
/// @param tree  The AST to analyze
/// @param name  the name of the function to search for
///
inline bool callsFunction(const ast::Tree& tree, const std::string& name);

/// @brief  For an AST node of a given type, search for and call a custom
///         const operator() which takes a const reference to every occurrence
///         of the specified node type.
///
/// @param tree  The AST to run over
/// @param op    The operator to call on every found AST node of type NodeT
///
template <typename NodeT, typename OpT>
inline void visitNodeType(const ast::Tree& tree, const OpT& op);


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


inline bool usesAttribute(const ast::Tree& tree, const std::string& name)
{
    bool found = false;
    visitNodeType<ast::Attribute>(tree,
        [&](const ast::Attribute& node) {
            if (found || node.mName != name) return;
            found = true;
        });

    return found;
}

inline bool writesToAttribute(const ast::Tree& tree, const std::string& name)
{
    bool found = false;
    visitNodeType<ast::AssignExpression>(tree,
        [&](const ast::AssignExpression& node) {
            if (found || node.mVariable->mName != name) return;
            const auto attribute =
                std::dynamic_pointer_cast<ast::Attribute>(node.mVariable);
            if (attribute) found = true;
        });

    return found;
}

inline bool callsFunction(const ast::Tree& tree, const std::string& name)
{
    bool found = false;
    visitNodeType<ast::FunctionCall>(tree,
        [&](const ast::FunctionCall& node) {
            if (found || node.mFunction != name) return;
            found = true;
        });

    return found;
}

template <typename NodeT, typename OpT>
struct VisitNodeType : public ast::Visitor
{
    VisitNodeType(const OpT& op) : mOp(op) {}
    ~VisitNodeType() override = default;
    inline void visit(const NodeT& node) override final { mOp(node); }
private:
    const OpT& mOp;
};

template <typename NodeT, typename OpT>
inline void visitNodeType(const ast::Tree& tree, const OpT& op)
{
    VisitNodeType<NodeT, OpT> visitOp(op);
    tree.accept(visitOp);
}

}
}
}
}

#endif // OPENVDB_AX_COMPILER_AST_SCANNERS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )

