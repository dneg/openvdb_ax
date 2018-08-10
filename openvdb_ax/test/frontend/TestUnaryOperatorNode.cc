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

enum BehaviourFlags
{
    Pass = unittest_util::ExpectedBase::Pass,
    Plus = int16_t(Pass) << 1,
    Minus = int16_t(Plus) << 1,
    Not = int16_t(Minus) << 1,
    Bitnot = int16_t(Not) << 1
};

#define EXPECTED_PASS(Flags, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<>(BehaviourFlags::Pass | Flags, Count))

static const unittest_util::CodeTests tests =
{
    { "-a;",          EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "+a;",          EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "!a;",          EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "~a;",          EXPECTED_PASS(BehaviourFlags::Bitnot, 1) },
    { "~~a;",         EXPECTED_PASS(BehaviourFlags::Bitnot, 2) },
    { "!~a;",         EXPECTED_PASS(BehaviourFlags::Not, 2) },
    { "!!a;",         EXPECTED_PASS(BehaviourFlags::Not, 2) },
    { "+-a;",         EXPECTED_PASS(BehaviourFlags::Plus, 2) },
    { "-+a;",         EXPECTED_PASS(BehaviourFlags::Minus, 2) },
    { "!-a;",         EXPECTED_PASS(BehaviourFlags::Not, 2) },
    { "!!!a;",        EXPECTED_PASS(BehaviourFlags::Not, 3) },
    { "~~~a;",        EXPECTED_PASS(BehaviourFlags::Bitnot, 3) },
    { "-(a+b);",      EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "!function();", EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "~a+b;",        EXPECTED_PASS(BehaviourFlags::Bitnot, 1) },
    { "~a;",          EXPECTED_PASS(BehaviourFlags::Bitnot, 1) },
    { "-@a;",         EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "!v@a;",        EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "~v@a;",        EXPECTED_PASS(BehaviourFlags::Bitnot, 1) },
    { "+int(a);",     EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "-(float(a));", EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "!a.x;",        EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "-a.x;",        EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "-++a;",        EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "-a.x=a;",      EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "!a=a;",        EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "!{a,b,c};",    EXPECTED_PASS(BehaviourFlags::Not, 1) },
    { "~{a,b,c};",    EXPECTED_PASS(BehaviourFlags::Bitnot, 1) },
    { "-{a,b,c};",    EXPECTED_PASS(BehaviourFlags::Minus, 1) },
};

struct UnaryVisitor : public openvdb::ax::ast::Visitor
{
    UnaryVisitor()
        : mCount(0), mNode(nullptr) {}
    ~UnaryVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::UnaryOperator& node) override final {
        ++mCount;
        mNode = &node;
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::UnaryOperator* mNode;
};

}

class TestUnaryOperatorNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestUnaryOperatorNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestUnaryOperatorNode);

void
TestUnaryOperatorNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (!behaviour->passes()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        UnaryVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            behaviour->count(), visitor.mCount);

        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node Expression", code), visitor.mNode->mExpression);

        if (behaviour->hasFlag(BehaviourFlags::Plus)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected + unary operation", code),
                visitor.mNode->mOperation, tokens::PLUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Minus)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected - unary operation", code),
                visitor.mNode->mOperation, tokens::MINUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Not)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected ! unary operation", code),
                visitor.mNode->mOperation, tokens::NOT);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Bitnot)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected ~ unary operation", code),
                visitor.mNode->mOperation, tokens::BITNOT);
        }
        else {
            CPPUNIT_FAIL(ERROR_MSG("Expected unary operation", code));
        }
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
