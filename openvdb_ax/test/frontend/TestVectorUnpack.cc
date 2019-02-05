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
    Pass = unittest_util::ExpectedBase::Pass,
    AttributeVectorUnpack = int16_t(Pass) << 1,
    LocalVectorUnpack = int16_t(AttributeVectorUnpack) << 1,
};

class ExpectedUnpack : public unittest_util::ExpectedBase
{
public:
    using Ptr = std::shared_ptr<ExpectedUnpack>;
    ExpectedUnpack(const int16_t flags, const size_t index = 0)
        : ExpectedBase(flags, 0)
        , mIndex(index) {}

    std::string type() const override
    {
        return "VectorUnpack";
    }

    short getIndex() const
    {
        return mIndex;
    }

private:
    const short mIndex = 0;
};

#define EXPECTED_PASS(Flags, Index) \
    ExpectedUnpack::Ptr(new ExpectedUnpack(Pass | Flags, short(Index)))

using CodeTests = std::vector<std::pair<std::string, ExpectedUnpack::Ptr>>;

static const CodeTests tests =
{
    { "a.x;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,0) },
    { "a.y;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,1) },
    { "a.z;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,2) },
    { "a.r;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,0) },
    { "a.g;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,1) },
    { "a.b;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,2) },
    { "x.x;",  EXPECTED_PASS(BehaviourFlags::LocalVectorUnpack,0) },
    { "@x.x;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,0) },
    { "@a.x;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,0) },
    { "@b.y;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,1) },
    { "@c.z;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,2) },
    { "@a.r;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,0) },
    { "@a.g;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,1) },
    { "@a.b;", EXPECTED_PASS(BehaviourFlags::AttributeVectorUnpack,2) },
};

struct VectorUnpackVisitor : public openvdb::ax::ast::Visitor
{
    ~VectorUnpackVisitor() override = default;

    void visit(const openvdb::ax::ast::VectorUnpack& node) override final
    {
        mCount++;
        mNode = &node;
    }

    size_t mCount = 0;
    const openvdb::ax::ast::VectorUnpack* mNode = nullptr;
};

}

class TestVectorUnpack : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestVectorUnpack);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestVectorUnpack);

void TestVectorUnpack::testASTNode()
{
    for (const auto& test : tests) {
        const ExpectedUnpack::Ptr behaviour = test.second;

        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());

        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), tree.get() != nullptr);

        VectorUnpackVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            size_t(1), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode != nullptr);

        openvdb::ax::ast::Expression::Ptr expression = visitor.mNode->mExpression;
        if (behaviour->hasFlag(BehaviourFlags::AttributeVectorUnpack)) {
            CPPUNIT_ASSERT(expression);

            // check that the expression is an AttributeValue
            openvdb::ax::ast::AttributeValue* attributeVal =
                dynamic_cast<openvdb::ax::ast::AttributeValue*>(expression.get());
            CPPUNIT_ASSERT(attributeVal);

            // check element index
            CPPUNIT_ASSERT_EQUAL(behaviour->getIndex(), visitor.mNode->mIndex);

        }
        if (behaviour->hasFlag(BehaviourFlags::LocalVectorUnpack)) {
            CPPUNIT_ASSERT(expression);

            // check that the expression is a LocalValue
            openvdb::ax::ast::LocalValue* localValue =
                dynamic_cast<openvdb::ax::ast::LocalValue*>(expression.get());
            CPPUNIT_ASSERT(localValue);

            // check element index
            CPPUNIT_ASSERT_EQUAL(behaviour->getIndex(), visitor.mNode->mIndex);
        }

    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
