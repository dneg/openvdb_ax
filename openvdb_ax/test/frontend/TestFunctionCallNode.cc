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

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

#include <string>

namespace {

struct ExpectedFunctionCall : public unittest_util::ExpectedBase
{
    using Ptr = std::shared_ptr<ExpectedFunctionCall>;
    ExpectedFunctionCall(const size_t flags = 0,
                         const size_t count = 0,
                         const std::string& name = "",
                         const size_t arguments = 0)
        : unittest_util::ExpectedBase(flags, count)
        , mName(name)
        , mNumArguments(arguments) {}

    std::string type() const override  { return ""; }
    const std::string& name() const { return mName; }
    size_t args() const { return mNumArguments; }
private:
    const std::string mName;
    const size_t mNumArguments;
};

#define EXPECTED_PASS(Count, Name, Args) \
    unittest_util::ExpectedBase::Ptr(new ExpectedFunctionCall(unittest_util::ExpectedBase::Pass, Count, Name, Args))

static const unittest_util::CodeTests tests =
{
    { "function();",            EXPECTED_PASS(1, "function", 0) },
    { "FUNCTION();",            EXPECTED_PASS(1, "FUNCTION", 0) },
    { "name();",                EXPECTED_PASS(1, "name"    , 0) },
    { "f(a);",                  EXPECTED_PASS(1, "f"       , 1) },
    { "a(a,1);",                EXPECTED_PASS(1, "a"       , 2) },
    { "function(1);",           EXPECTED_PASS(1, "function", 1) },
    { "function(\"string\");",  EXPECTED_PASS(1, "function", 1) },
    { "function(true);",        EXPECTED_PASS(1, "function", 1) },
    { "function({a,b,c});",     EXPECTED_PASS(1, "function", 1) },
    { "function(@a);",          EXPECTED_PASS(1, "function", 1) },
    { "function(++a);",         EXPECTED_PASS(1, "function", 1) },
    { "function(~a);",          EXPECTED_PASS(1, "function", 1) },
    { "function((a));",         EXPECTED_PASS(1, "function", 1) },
    { "function(function());",  EXPECTED_PASS(2, "function", 1) },
    { "function(a=a);",         EXPECTED_PASS(1, "function", 1) },
    { "function(a==a);",        EXPECTED_PASS(1, "function", 1) },
    { "function(a.x=a);",       EXPECTED_PASS(1, "function", 1) },
    { "function(bool(a));",     EXPECTED_PASS(1, "function", 1) },
    { "function(a.x);",         EXPECTED_PASS(1, "function", 1) },
    { "function(a,b,c,d,e,f);", EXPECTED_PASS(1, "function", 6) },
};

struct FunctionCallVisitor : public openvdb::ax::ast::Visitor
{
    FunctionCallVisitor()
        : mCount(0), mNode(nullptr) {}
    ~FunctionCallVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::FunctionCall& node) override final {
        ++mCount;
        mNode = &node;
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::FunctionCall* mNode;
};

}

class TestFunctionCallNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestFunctionCallNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFunctionCallNode);

void
TestFunctionCallNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const ExpectedFunctionCall::Ptr behaviour =
            std::static_pointer_cast<ExpectedFunctionCall>(test.second);
        if (!behaviour->passes()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        FunctionCallVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            behaviour->count(), visitor.mCount);

        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node Expression", code), visitor.mNode->mArguments);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected argument list size", code),
            behaviour->args(), visitor.mNode->mArguments->mList.size());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected function name", code),
            behaviour->name(), visitor.mNode->mFunction);
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
