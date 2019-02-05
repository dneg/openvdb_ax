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

#define EXPECTED_PASS(Count) \
    unittest_util::ExpectedBase::Ptr( new unittest_util::ExpectedType<>(unittest_util::ExpectedBase::Pass, Count))

static const unittest_util::CodeTests tests =
{
    { "{1, 2, {1,2,3}};",               EXPECTED_PASS(2) },
    { "{{a,b,c}, {a,b,c}, {a,b,c}};",   EXPECTED_PASS(4) },
    { "{1.0,2.0,3.0};",                 EXPECTED_PASS(1) },
    { "{(a),(b),(c)};",                 EXPECTED_PASS(1) },
    { "{@x,++z,3.0};",                  EXPECTED_PASS(1) },
    { "{@x,++z,\"bar\"};",              EXPECTED_PASS(1) },
    { "{a+b,c+d,e+f};",                 EXPECTED_PASS(1) },
    { "{!a,~c,-b};",                    EXPECTED_PASS(1) },
    { "a.x=a;",                         EXPECTED_PASS(1) },
    { "{a.x=a,b.y=b,c.z=c};",           EXPECTED_PASS(4) },
    { "{a=a,b=b,c=c};",                 EXPECTED_PASS(1) },
    { "{int(a), float(b), double(c)};", EXPECTED_PASS(1) },
};

struct VectorPackVisitor : public openvdb::ax::ast::Visitor
{
    ~VectorPackVisitor() override = default;

    void visit(const openvdb::ax::ast::VectorPack& node) override
    {
        mCount++;
        mNode = &node;
    }

    size_t mCount = 0;
    const openvdb::ax::ast::VectorPack* mNode = nullptr;
};

}

class TestVectorPack : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestVectorPack);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestVectorPack);

void TestVectorPack::testASTNode()
{
    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), tree.get() != nullptr);

        VectorPackVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            behaviour->count(), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode != nullptr);

        CPPUNIT_ASSERT(visitor.mNode->mValue1.get() != nullptr);
        CPPUNIT_ASSERT(visitor.mNode->mValue2.get() != nullptr);
        CPPUNIT_ASSERT(visitor.mNode->mValue3.get() != nullptr);

    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
