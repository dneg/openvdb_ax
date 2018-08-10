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
    Pass = unittest_util::ExpectedBase::Pass,    // else Fails
    AttributeCrement = int16_t(Pass) << 1,       // else LocalCrement
    Increment = int16_t(AttributeCrement) << 1,  // else Decrement
    Post = int16_t(Increment) << 1,              // else Precrement
};

#define EXPECTED_PASS(Flags, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<>(BehaviourFlags::Pass | Flags, Count))

// @note  successful AST tests which check the crement node must all operate on an attribute
//        or local names 'a' for post and pre increment checks

static const unittest_util::CodeTests tests =
{
    { "a++;",        EXPECTED_PASS(BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
    { "++a;",        EXPECTED_PASS(BehaviourFlags::Increment, 1) },
    { "-a++;",       EXPECTED_PASS(BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
    { "+a++;",       EXPECTED_PASS(BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
    { "+a--;",       EXPECTED_PASS(BehaviourFlags::Post, 1) },
    { "-a--;",       EXPECTED_PASS(BehaviourFlags::Post, 1) },
    { "s@a--;",      EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Post, 1) },
    { "f@a++;",      EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
    { "++f@a;",      EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Increment, 1) },
    { "-@a--;",      EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Post, 1) },
    { "++@a;",       EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Increment, 1) },
    { "@a++;",       EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
    { "-@a++;",      EXPECTED_PASS(BehaviourFlags::AttributeCrement | BehaviourFlags::Increment | BehaviourFlags::Post, 1) },
};

struct CrementVisitor : public openvdb::ax::ast::Visitor
{
    ~CrementVisitor() override = default;
    void visit(const openvdb::ax::ast::Crement& node) override final {
        mCount++;
        mNode = &node;
    }
    size_t mCount = 0;
    const openvdb::ax::ast::Crement* mNode = nullptr;
};

}

class TestCrementNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestCrementNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests) };
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCrementNode);

void
TestCrementNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        CrementVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected AST node count", code),
            behaviour->count(), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
            static_cast<bool>(visitor.mNode));

        Expression::Ptr expression = visitor.mNode->mExpression;
        CPPUNIT_ASSERT(static_cast<bool>(expression));

        if (behaviour->hasFlag(BehaviourFlags::AttributeCrement)) {
            // check attribute
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Expected attribute crement.", code),
                code.find("@") != std::string::npos);

            const AttributeValue::Ptr attribute =
                std::dynamic_pointer_cast<AttributeValue>(expression);

            CPPUNIT_ASSERT(static_cast<bool>(attribute));
            CPPUNIT_ASSERT(static_cast<bool>(attribute->mAttribute));
        }
        else {
            // check local
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Expected local crement.", code),
                code.find("@") == std::string::npos);

            const LocalValue::Ptr local =
                std::dynamic_pointer_cast<LocalValue>(expression);

            CPPUNIT_ASSERT(static_cast<bool>(local));
            CPPUNIT_ASSERT(static_cast<bool>(local->mLocal));
        }

        if (behaviour->hasFlag(BehaviourFlags::Increment)) {
            // check increment
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Expected increment.", code),
                code.find("++") != std::string::npos);

            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected increment.", code),
                Crement::Increment, visitor.mNode->mOperation);
        }
        else {
            // check decrement
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Expected decrement.", code),
                code.find("--") != std::string::npos);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected decrement.", code),
                Crement::Decrement, visitor.mNode->mOperation);
        }

        const size_t variable = code.find("a");
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Crement test is not crementing a variable 'a'.", code),
            variable != std::string::npos);

        const bool isPostCrement =
            (code.substr(variable+1, 2) == "++" ||
             code.substr(variable+1, 2) == "--");

        if (behaviour->hasFlag(BehaviourFlags::Post)) {
            // check post crement
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected post crement", code),
                isPostCrement, visitor.mNode->mPost);
        }
        else {
            // check pre crement
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected pre crement", code),
                isPostCrement, visitor.mNode->mPost);
        }
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
