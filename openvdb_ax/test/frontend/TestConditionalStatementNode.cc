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

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

#include <string>

namespace {

enum BehaviourFlags
{
    Pass = unittest_util::ExpectedBase::Pass,    // else Fails
};

#define EXPECTED_PASS(Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<>(BehaviourFlags::Pass, Count))

static const unittest_util::CodeTests tests =
{
    { "if ((a));",                                     EXPECTED_PASS(1) },
    { "if (function());",                              EXPECTED_PASS(1) },
    { "if (a+b);",                                     EXPECTED_PASS(1) },
    { "if (-a);",                                      EXPECTED_PASS(1) },
    { "if (a = 1);",                                   EXPECTED_PASS(1) },
    { "if (a.x = 1);",                                 EXPECTED_PASS(1) },
    { "if (a++);",                                     EXPECTED_PASS(1) },
    { "if (float(a));",                                EXPECTED_PASS(1) },
    { "if ({1.0, 2.0, 3.0});",                         EXPECTED_PASS(1) },
    { "if (a.z);",                                     EXPECTED_PASS(1) },
    { "if (1.0f);",                                    EXPECTED_PASS(1) },
    { "if (@a);",                                      EXPECTED_PASS(1) },
    { "if (@a=1);",                                    EXPECTED_PASS(1) },
    { "if (@a==1);",                                   EXPECTED_PASS(1) },
    { "if (a);",                                       EXPECTED_PASS(1) },
    { "if(a);",                                        EXPECTED_PASS(1) },
    { "if (a) {b;}",                                   EXPECTED_PASS(1) },
    { "if (a); else ;",                                EXPECTED_PASS(1) },
    { "if (a) {} else ;",                              EXPECTED_PASS(1) },
    { "if (a); else {}",                               EXPECTED_PASS(1) },
    { "if (a); else (b);",                             EXPECTED_PASS(1) },
    { "if (a) {} else {}",                             EXPECTED_PASS(1) },
    { "if (a) b = 1; else {}",                         EXPECTED_PASS(1) },
    { "if (a) {b = 1;} else {}",                       EXPECTED_PASS(1) },
    { "if (a); else (b);",                             EXPECTED_PASS(1) },
    { "if (a); else if(b) ;",                          EXPECTED_PASS(2) },
    { "if (a) if(b) ; else ;",                         EXPECTED_PASS(2) },
    { "if (a) if(b) {} else {} else ;",                EXPECTED_PASS(2) },
    { "if (a) if(b) {if (c) ; else ;} else {} else ;", EXPECTED_PASS(3) },
    { "if (a) if(b) {if (c) ;} else {}",               EXPECTED_PASS(3) },
    { "if (a) {} else if(b) {if (c) ;} else {}",       EXPECTED_PASS(3) },
    { "if (a); else {};",                              EXPECTED_PASS(1) },
};

struct ConditionalStatementVisitor : public openvdb::ax::ast::Visitor
{
    ~ConditionalStatementVisitor() override = default;
    void visit(const openvdb::ax::ast::ConditionalStatement& node) override final {
        mCount++;
        mNode = &node;
    }
    size_t mCount = 0;
    const openvdb::ax::ast::ConditionalStatement* mNode = nullptr;
};

}

class TestConditionalStatementNode : public CppUnit::TestCase
{
    public:

    CPPUNIT_TEST_SUITE(TestConditionalStatementNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestConditionalStatementNode);

void TestConditionalStatementNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        ConditionalStatementVisitor visitor;
        tree->accept(visitor);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected AST node count", code),
            behaviour->count(), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
            static_cast<bool>(visitor.mNode));

        CPPUNIT_ASSERT(visitor.mNode->mConditional.get() != nullptr);
        CPPUNIT_ASSERT(visitor.mNode->mThenBranch.get()  != nullptr);
        CPPUNIT_ASSERT(visitor.mNode->mElseBranch.get()  != nullptr);

    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
