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

#include "PrintTree.h"

#include "AST.h"
#include "Tokens.h"

#include <openvdb/Types.h>

#include <iostream>
#include <sstream>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace ast {

struct PrintVisitor : public ast::Visitor
{
    PrintVisitor(std::ostream& os);
    ~PrintVisitor() override = default;

    void init(const ast::Tree& node) override;
    void visit(const ast::Block& node) override;
    void visit(const ast::ConditionalStatement& node) override;
    void visit(const ast::AssignExpression& node) override;
    void visit(const ast::Crement& node) override;
    void visit(const ast::ExpressionList& node) override;
    void visit(const ast::UnaryOperator& node) override;
    void visit(const ast::BinaryOperator& node) override;
    void visit(const ast::Cast& node) override;
    void visit(const ast::FunctionCall& node) override;
    void visit(const ast::Attribute& node) override;
    void visit(const ast::AttributeValue& node) override;
    void visit(const ast::ExternalVariable& node) override;
    void visit(const ast::DeclareLocal& node) override;
    void visit(const ast::Local& node) override;
    void visit(const ast::LocalValue& node) override;
    void visit(const ast::VectorUnpack& node) override;
    void visit(const ast::VectorPack& node) override;

    void visit(const ast::Value<bool>& node) override
    {
        this->visitValue(node);
    }
    void visit(const ast::Value<int16_t>& node) override
    {
        this->visitValue(node);
    }
    void visit(const ast::Value<int32_t>& node) override
    {
        this->visitValue(node);
    }
    void visit(const ast::Value<int64_t>& node) override
    {
        this->visitValue(node);
    }
    void visit(const ast::Value<float>& node) override
    {
        this->visitValue(node);
    }
    void visit(const ast::Value<double>& node) override
    {
        this->visitValue(node);
    }

protected:
    template <typename T>
    void visitValue(const ast::Value<T>& node);
    void printIndent();

private:
    std::ostream& mOs;
    int mIndent;
    std::string mLastValue;
    std::string mLastAttr;
};

PrintVisitor::PrintVisitor(std::ostream& os)
    : mOs(os)
    , mIndent(0)
{
}

void PrintVisitor::init(const ast::Tree& node)
{
    printIndent();
    mOs << "\nTree\n";
    mIndent++;
}

void PrintVisitor::visit(const ast::Block& node)
{
    printIndent();
    mOs << "Block encountered: " << node.mList.size() << " number of statements" << std::endl;
}

void PrintVisitor::visit(const ast::ConditionalStatement& node)
{
    printIndent();
    mOs << "Conditional Statement: " << ((node.mElseBranch) ? "two branches " : "one branch") << std::endl;
}


void PrintVisitor::visit(const ast::AssignExpression& node)
{
    printIndent();
    mOs << "AssignExpression: " << mLastAttr << " = " << mLastValue << std::endl;
}

void PrintVisitor::visit(const ast::Crement& node)
{
    printIndent();
    if (node.mPost) mOs << "Post-";
    else           mOs << "Pre-";
    if (node.mOperation == ast::Crement::Increment)
                   mOs << "increment ";
    else           mOs << "decrement ";
    mOs << mLastAttr << std::endl;
}

void PrintVisitor::visit(const ast::ExpressionList& node)
{
    printIndent();
    mOs << "ExpressionList " << std::endl;
}

void PrintVisitor::visit(const ast::UnaryOperator& node)
{
    printIndent();
    mOs << "UnaryOperator: " << tokens::operatorNameFromToken(node.mOperation) << std::endl;
}

void PrintVisitor::visit(const ast::BinaryOperator& node)
{
    printIndent();
    mOs << "BinaryOperator: " << tokens::operatorNameFromToken(node.mOperation) << std::endl;
}

void PrintVisitor::visit(const ast::Cast& node)
{
    printIndent();
    mOs << "Cast to: " << node.mType << std::endl;
}

void PrintVisitor::visit(const ast::FunctionCall& node)
{
    printIndent();
    mOs << "FunctionCall: " << node.mFunction << std::endl;
}

void PrintVisitor::visit(const ast::Attribute& node)
{
    printIndent();
    const std::string inferred  = node.mTypeInferred ? " - inferred": "";
    mLastAttr = "@(" + node.mType + inferred + ") " + node.mName;

    mOs << "Attribute: " << mLastAttr << std::endl;
}

void PrintVisitor::visit(const ast::AttributeValue& node)
{
    printIndent();
    mOs << "AttributeValue " << std::endl;
}

void PrintVisitor::visit(const ast::DeclareLocal& node)
{
    printIndent();
    mLastAttr = node.mName;

    mOs << "Local Declared: " << mLastAttr << std::endl;
}

void PrintVisitor::visit(const ast::Local& node)
{
    printIndent();
    mOs << "Local: " << node.mName << std::endl;
}

void PrintVisitor::visit(const ast::LocalValue& node)
{
    printIndent();
    mOs << "LocalValue " << std::endl;
}

void PrintVisitor::visit(const ast::VectorUnpack& node)
{
    printIndent();
    mOs << "VectorUnpack " << std::endl;
}

void PrintVisitor::visit(const ast::VectorPack& node)
{
    printIndent();
    mOs << "VectorPack " << std::endl;
}

void PrintVisitor::visit(const ast::ExternalVariable& node)
{
    printIndent();
    mOs << "External Variable " << node.mName << std::endl;
}

template <typename T>
void PrintVisitor::visitValue(const ast::Value<T>& node)
{
    std::stringstream ss;
    ss << typeNameAsString<T>() << "(" << node.mValue << ")";
    mLastValue = ss.str();

    printIndent();
    mOs << "Value: " << mLastValue << std::endl;
}

void PrintVisitor::printIndent()
{
    for (int i(0); i != mIndent; ++i) mOs << '\t';
}


////////////////////////////////////////////////////////////////////////////////


void print(const ast::Tree& tree, std::ostream& os)
{
    PrintVisitor visitor(os);
    tree.accept(visitor);
}


} // namespace ast
} // namespace ax

}
} // namespace openvdb

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )

