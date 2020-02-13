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

#include "Scanners.h"
#include "Visitor.h"

#include <string>
#include <unordered_map>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

bool usesAttribute(const ast::Node& node,
    const std::string& name,
    const tokens::CoreType type)
{
    bool found = false;
    visitNodeType<ast::Attribute>(node,
        [&](const ast::Attribute& attrib) -> bool {
            assert(!found);
            if (type != tokens::UNKNOWN) {
                if (attrib.type() != type) return true;
            }
            if (attrib.name() != name) return true;
            found = true;
            return false;
        });

    return found;
}

bool writesToAttribute(const ast::Node& node,
    const std::string& name,
    const tokens::CoreType type)
{
    std::vector<const ast::Variable*> vars;
    catalogueVariables(node, nullptr, &vars, &vars, false, true);

    // See if any attributes in the result vec match the given name/type
    for (const ast::Variable* var : vars) {
        assert(var->isType<ast::Attribute>());
        const ast::Attribute* attrib = static_cast<const ast::Attribute*>(var);
        if (type != tokens::UNKNOWN) {
            if (attrib->type() != type) continue;
        }
        if (attrib->name() != name) continue;
        return true;
    }

    return false;
}

void catalogueVariables(const ast::Node& node,
        std::vector<const ast::Variable*>* readOnly,
        std::vector<const ast::Variable*>* writeOnly,
        std::vector<const ast::Variable*>* readWrite,
        const bool locals,
        const bool attributes)
{
    std::vector<const ast::Variable*> vars;

    if (locals) {
        using ListT = openvdb::TypeList<ast::Local, ast::DeclareLocal>;
        collectNodeTypes<ListT>(node, vars);
    }
    if (attributes) {
        collectNodeType<ast::Attribute>(node, vars);
    }

    for (const ast::Variable* var : vars) {
        // traverse upwards, see if we're embedded in an assign or crement expression
        const ast::Node* child = var;
        const ast::Node* parent = child->parent();
        bool read = false, write = false;
        while (parent && !(write && read)) {
            const ast::Node::NodeType type = parent->nodetype();
            // crement operations read and write
            if (type == ast::Node::CrementNode) {
                read = write = true;
            }
            else if (type == ast::Node::AssignExpressionNode) {
                const ast::AssignExpression* assignment =
                    static_cast<const ast::AssignExpression*>(parent);
                if (assignment->lhs() == child) {
                    if (assignment->isCompound()) {
                        // +=, *=, /= etc
                        read = write = true;
                    }
                    else {
                        // op = op
                        write = true;
                    }
                }
                else {
                    read = true;
                }
            }
            else if (type == ast::Node::FunctionCallNode) {
                // @todo  We currently can't detect if attributes are being passed by
                //   pointer and being modified automatically. This is a major limitation
                //   as it means any attribute passed into any function directly must
                //   be marked as writeable
                read = write = true;
            }
            else {
                read = true;
            }
            child = parent;
            parent = child->parent();
        }

        assert(read || write);
        if (readWrite && read && write)  readWrite->emplace_back(var);
        if (readOnly && read && !write)  readOnly->emplace_back(var);
        if (writeOnly && !read && write) writeOnly->emplace_back(var);
    }
}

void catalogueAttributeTokens(const ast::Node& node,
        std::vector<std::string>* readOnly,
        std::vector<std::string>* writeOnly,
        std::vector<std::string>* readWrite)
{
    std::vector<const ast::Variable*> readOnlyVars;
    std::vector<const ast::Variable*> writeOnlyVars;
    std::vector<const ast::Variable*> readWriteVars;
    catalogueVariables(node,
        (readOnly ? &readOnlyVars : nullptr),
        (writeOnly ? &writeOnlyVars : nullptr),
        (readWrite ? &readWriteVars : nullptr),
        false, // locals
        true); // attributes

    // fill a single map with the access patterns for all attributes
    // .first = read, .second = write
    std::unordered_map<std::string, std::pair<bool,bool>> accessmap;

    auto addAccesses = [&](const std::vector<const ast::Variable*>& vars,
        const bool read,
        const bool write)
    {
        for (const ast::Variable* var : vars) {
            assert(var->isType<ast::Attribute>());
            const ast::Attribute* attrib = static_cast<const ast::Attribute*>(var);
            auto& access = accessmap[attrib->tokenname()];
            access.first |= read;
            access.second |= write;
        }
    };

    addAccesses(readWriteVars, true, true);
    addAccesses(writeOnlyVars, false, true);
    addAccesses(readOnlyVars, true, false);

    // set the results from the access map
    for (const auto& result : accessmap) {
        const std::pair<bool,bool>& pair = result.second;
        if (readWrite && pair.first && pair.second) {
            readWrite->emplace_back(result.first);
        }
        else if (writeOnly && !pair.first && pair.second) {
            writeOnly->emplace_back(result.first);
        }
        else if (readOnly && pair.first && !pair.second) {
            readOnly->emplace_back(result.first);
        }
    }
}

void variableDependencies(const ast::Variable& var,
        std::vector<const ast::Variable*>& dependencies)
{
    // Get the root node
    const ast::Node* root = &var;
    while (const ast::Node* parent = root->parent()) {
        root = parent;
    }

    // collect all occurrences of this var up to and including
    // it's current usage, terminating traversal afterwards
    std::vector<const ast::Variable*> usage;
    const bool attributeVisit =
        (var.nodetype() == ast::Node::AttributeNode);
    const ast::Attribute* asAttrib = attributeVisit ?
        static_cast<const ast::Attribute*>(&var) : nullptr;

    if (attributeVisit) {
        visitNodeType<ast::Attribute>(*root,
            [&](const ast::Attribute& attrib) -> bool {
                if (attrib.tokenname() == asAttrib->tokenname()) {
                    usage.emplace_back(&attrib);
                }
                return &attrib != &var;
            });
    }
    else {
        visitNodeType<ast::Local>(*root,
            [&](const ast::Local& local) -> bool {
                if (local.name() == var.name()) {
                    usage.emplace_back(&local);
                }
                return &local != &var;
            });
        // also visit declarations
        visitNodeType<ast::DeclareLocal>(*root,
            [&](const ast::DeclareLocal& dcl) -> bool {
                if (dcl.name() == var.name()) {
                    usage.emplace_back(&dcl);
                }
                return &dcl != &var;
            });
    }

    // The list of nodes which can be considered dependencies to collect
    using ListT = openvdb::TypeList<
        ast::Attribute,
        ast::Local,
        ast::ExternalVariable>;

    // small lambda to check to see if a dep is already being tracked
    auto hasDep = [&](const ast::Variable* dep) -> bool {
        return (std::find(dependencies.cbegin(), dependencies.cend(), dep) !=
            dependencies.cend());
    };

    // recursively traverse all usages and resolve dependencies
    for (const auto& use : usage)
    {
        const ast::Node* child = use;
        // track writable for conditionals
        bool written = false;
        while (const ast::Node* parent = child->parent()) {
            const ast::Node::NodeType type = parent->nodetype();
            if (type == ast::Node::CrementNode) {
                written = true;
                if (!hasDep(use)) {
                    dependencies.emplace_back(use);
                }
            }
            else if (type == ast::Node::ConditionalStatementNode) {
                const ast::ConditionalStatement* conditional =
                    static_cast<const ast::ConditionalStatement*>(parent);
                // traverse down and collect variables
                std::vector<const ast::Variable*> vars;
                collectNodeTypes<ListT>(*conditional->condition(), vars);
                // find next deps
                for (const ast::Variable* dep : vars) {
                    // don't add this dep if it's not being written to. Unlike
                    // all other visits, the conditionals dictate program flow.
                    // Values in the conditional expression only link to the
                    // current usage if the current usage is being modified
                    if (!written || hasDep(dep)) continue;
                    dependencies.emplace_back(dep);
                    variableDependencies(*dep, dependencies);
                }
            }
            else if (type == ast::Node::LoopNode) {
                const ast::Loop* loop =
                    static_cast<const ast::Loop*>(parent);
                // traverse down and collect variables
                std::vector<const ast::Variable*> vars;

                const ast::Statement* condition = loop->condition();
                // if the condition is an expression list the last element determines flow
                if (condition->nodetype() == ast::Node::NodeType::ExpressionListNode) {
                    const ast::ExpressionList*
                        exprList = static_cast<const ast::ExpressionList*>(condition);
                    if (!exprList->empty()) {
                        const ast::Expression* lastExpression = exprList->child(exprList->size()-1);
                        collectNodeTypes<ListT>(*lastExpression, vars);
                    }
                }
                else {
                    collectNodeTypes<ListT>(*condition, vars);
                }
                // find next deps
                for (const ast::Variable* dep : vars) {
                    // don't add this dep if it's not being written to. Unlike
                    // all other visits, the conditionals dictate program flow.
                    // Values in the conditional expression only link to the
                    // current usage if the current usage is being modified
                    if (!written || hasDep(dep)) continue;
                    dependencies.emplace_back(dep);
                    variableDependencies(*dep, dependencies);
                }
            }
            else if (type == ast::Node::AssignExpressionNode) {
                const ast::AssignExpression* assignment =
                    static_cast<const ast::AssignExpression*>(parent);
                if (assignment->lhs() == child) {
                    written = true;
                    // traverse down and collect variables
                    std::vector<const ast::Variable*> vars;
                    collectNodeTypes<ListT>(*assignment->rhs(), vars);
                    // find next deps
                    for (const ast::Variable* dep : vars) {
                        if (hasDep(dep)) continue;
                        dependencies.emplace_back(dep);
                        variableDependencies(*dep, dependencies);
                    }
                }
            }
            else if (type == ast::Node::FunctionCallNode) {
                written = true;
                // @todo  We currently can't detect if attributes are being passed by
                //   pointer and being modified automatically. We have to link this
                //   attribute to any other attribute passes into the function
                const ast::FunctionCall* call =
                    static_cast<const ast::FunctionCall*>(parent);
                // traverse down and collect variables
                std::vector<const ast::Variable*> vars;
                collectNodeTypes<ListT>(*call->args(), vars);
                // only append dependencies here if they havent already been visited
                // due to recursion issues
                for (const ast::Variable* dep : vars) {
                    // make sure the dep doesn't already exist in the container, otherwise
                    // we can get into issues where functions with multiple arguments
                    // constantly try to check themselves
                    // @note  should be removed with function refactoring
                    if (hasDep(dep)) continue;
                    dependencies.emplace_back(dep);
                    variableDependencies(*dep, dependencies);
                }
            }
            child = parent;
        }
    }
}

void attributeDependencyTokens(const ast::Tree& tree,
        const std::string& name,
        const tokens::CoreType type,
        std::vector<std::string>& dependencies)
{
    const std::string token = ast::Attribute::tokenFromNameType(name, type);
    const ast::Variable* var = lastUse(tree, token);
    if (!var) return;
    assert(var->isType<ast::Attribute>());

    std::vector<const ast::Variable*> deps;
    variableDependencies(*var, deps);

    for (const auto& dep : deps) {
        if (dep->nodetype() != ast::Node::AttributeNode) continue;
        dependencies.emplace_back(static_cast<const ast::Attribute*>(dep)->tokenname());
    }

    std::sort(dependencies.begin(), dependencies.end());
    auto iter = std::unique(dependencies.begin(), dependencies.end());
    dependencies.erase(iter, dependencies.end());
}

template <bool First>
struct UseVisitor :
    public ast::Visitor<UseVisitor<First>>
{
    using ast::Visitor<UseVisitor<First>>::traverse;
    using ast::Visitor<UseVisitor<First>>::visit;

    // reverse the ast traversal if !First
    inline bool reverseChildVisits() const { return !First; }

    UseVisitor(const std::string& tokenOrName)
        : mToken(tokenOrName)
        , mAttribute(false)
        , mVar(nullptr) {
            // rebuild the expected token if necessary
            std::string name, type;
            mAttribute = ast::Attribute::nametypeFromToken(mToken, &name, &type);
            if (mAttribute) {
                mToken = type + ast::Attribute::symbolseparator() + name;
            }
        }
    ~UseVisitor() = default;

    inline bool visit(const ast::Attribute* node) {
        if (!mAttribute) return true;
        if (node->tokenname() != mToken) return true;
        mVar = node;
        return false;
    }
    inline bool visit(const ast::Local* node) {
        if (mAttribute) return true;
        if (node->name() != mToken) return true;
        mVar = node;
        return false;
    }
    inline bool visit(const ast::DeclareLocal* node) {
        if (mAttribute) return true;
        if (node->name() != mToken) return true;
        mVar = node;
        return false;
    }

    const ast::Variable* var() const { return mVar; }
private:
    std::string mToken;
    bool mAttribute;
    const ast::Variable* mVar;
};

const ast::Variable* firstUse(const ast::Node& node, const std::string& tokenOrName)
{
    UseVisitor<true> visitor(tokenOrName);
    visitor.traverse(&node);
    return visitor.var();
}

const ast::Variable* lastUse(const ast::Node& node, const std::string& tokenOrName)
{
    UseVisitor<false> visitor(tokenOrName);
    visitor.traverse(&node);
    return visitor.var();
}

bool callsFunction(const ast::Node& node, const std::string& name)
{
    bool found = false;
    visitNodeType<ast::FunctionCall>(node,
        [&](const ast::FunctionCall& call) -> bool {
            if (call.name() != name) return true;
            found = true;
            return false;
        });

    return found;
}

void linearize(const ast::Node& node, std::vector<const ast::Node*>& list)
{
    collectNodeType<ast::Node>(node, list);
}

}
}
}
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )

