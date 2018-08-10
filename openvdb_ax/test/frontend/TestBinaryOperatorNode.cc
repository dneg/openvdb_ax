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
    Plus = size_t(Pass) << 1,
    Minus = size_t(Plus) << 1,
    Multiply = size_t(Minus) << 1,
    Divide = size_t(Multiply) << 1,
    Modulo = size_t(Divide) << 1,
    And = size_t(Modulo) << 1,
    Or = size_t(And) << 1,
    Equalsequals = size_t(Or) << 1,
    Notequals = size_t(Equalsequals) << 1,
    Morethan = size_t(Notequals) << 1,
    Lessthan = size_t(Morethan) << 1,
    Morethanorequal = size_t(Lessthan) << 1,
    Lessthanorequal = size_t(Morethanorequal) << 1,
    Bitand = size_t(Lessthanorequal) << 1,
    Bitor = size_t(Bitand) << 1,
    Bitxor = size_t(Bitor) << 1
};

#define EXPECTED_PASS(Flags, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<>(BehaviourFlags::Pass | Flags, Count))

static const unittest_util::CodeTests tests =
{
    { "a + a;",                   EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "a - a;",                   EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "a * a;",                   EXPECTED_PASS(BehaviourFlags::Multiply, 1) },
    { "a / a;",                   EXPECTED_PASS(BehaviourFlags::Divide, 1) },
    { "a % a;",                   EXPECTED_PASS(BehaviourFlags::Modulo, 1) },
    { "a & a;",                   EXPECTED_PASS(BehaviourFlags::Bitand, 1) },
    { "a | a;",                   EXPECTED_PASS(BehaviourFlags::Bitor, 1) },
    { "a ^ a;",                   EXPECTED_PASS(BehaviourFlags::Bitxor, 1) },
    { "a && a;",                  EXPECTED_PASS(BehaviourFlags::And, 1) },
    { "a || a;",                  EXPECTED_PASS(BehaviourFlags::Or, 1) },
    { "a == a;",                  EXPECTED_PASS(BehaviourFlags::Equalsequals, 1) },
    { "a != a;",                  EXPECTED_PASS(BehaviourFlags::Notequals, 1) },
    { "a > a;",                   EXPECTED_PASS(BehaviourFlags::Morethan, 1) },
    { "a < a;",                   EXPECTED_PASS(BehaviourFlags::Lessthan, 1) },
    { "a >= a;",                  EXPECTED_PASS(BehaviourFlags::Morethanorequal, 1) },
    { "a <= a;",                  EXPECTED_PASS(BehaviourFlags::Lessthanorequal, 1) },
    { "(a) + (a);",               EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "function() + function();", EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "a + a + a;",               EXPECTED_PASS(BehaviourFlags::Plus, 2) },
    { "~a + !a;",                 EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "a + a += a;",              EXPECTED_PASS(BehaviourFlags::Plus, 2) },
    { "a += a + a;",              EXPECTED_PASS(BehaviourFlags::Plus, 2) },
    { "a = a + a = a;",           EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "a + a.x += a;",            EXPECTED_PASS(BehaviourFlags::Plus, 2) },
    { "++a - --a;",               EXPECTED_PASS(BehaviourFlags::Minus, 1) },
    { "a-- + a++;",               EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "int(a) + float(a);",       EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "{a,b,c} + {a,b,c};",       EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "a.x + a.y;",               EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "0 + 0;",                   EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "0.0f + 0;",                EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "@a + @a;",                 EXPECTED_PASS(BehaviourFlags::Plus, 1) },
    { "\"\" + \"\";",             EXPECTED_PASS(BehaviourFlags::Plus, 1) },
};

struct BinaryVisitor : public openvdb::ax::ast::Visitor
{
    BinaryVisitor()
        : mCount(0), mNode(nullptr) {}
    ~BinaryVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::BinaryOperator& node) override final {
        ++mCount;
        mNode = &node;
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::BinaryOperator* mNode;
};

}

class TestBinaryOperatorNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestBinaryOperatorNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestBinaryOperatorNode);

void
TestBinaryOperatorNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (!behaviour->passes()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        BinaryVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            behaviour->count(), visitor.mCount);

        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node Expression", code), visitor.mNode->mLeft);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node Expression", code), visitor.mNode->mRight);

        if (behaviour->hasFlag(BehaviourFlags::Plus)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected + binary operation", code),
                visitor.mNode->mOperation, tokens::PLUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Minus)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected - binary operation", code),
                visitor.mNode->mOperation, tokens::MINUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Multiply)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected * binary operation", code),
                visitor.mNode->mOperation, tokens::MULTIPLY);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Divide)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected / binary operation", code),
                visitor.mNode->mOperation, tokens::DIVIDE);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Modulo)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected % binary operation", code),
                visitor.mNode->mOperation, tokens::MODULO);
        }
        else if (behaviour->hasFlag(BehaviourFlags::And)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected && binary operation", code),
                visitor.mNode->mOperation, tokens::AND);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Or)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected || binary operation", code),
                visitor.mNode->mOperation, tokens::OR);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Equalsequals)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected == binary operation", code),
                visitor.mNode->mOperation, tokens::EQUALSEQUALS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Notequals)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected != binary operation", code),
                visitor.mNode->mOperation, tokens::NOTEQUALS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Morethan)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected > binary operation", code),
                visitor.mNode->mOperation, tokens::MORETHAN);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Lessthan)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected < binary operation", code),
                visitor.mNode->mOperation, tokens::LESSTHAN);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Morethanorequal)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected >= binary operation", code),
                visitor.mNode->mOperation, tokens::MORETHANOREQUAL);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Lessthanorequal)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected <= binary operation", code),
                visitor.mNode->mOperation, tokens::LESSTHANOREQUAL);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Bitand)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected & binary operation", code),
                visitor.mNode->mOperation, tokens::BITAND);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Bitor)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected | binary operation", code),
                visitor.mNode->mOperation, tokens::BITOR);
        }
        else if (behaviour->hasFlag(BehaviourFlags::Bitxor)) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected ^ binary operation", code),
                visitor.mNode->mOperation, tokens::BITXOR);
        }
        else {
            CPPUNIT_FAIL(ERROR_MSG("Expected binary operation", code));
        }
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
