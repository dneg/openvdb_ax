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
 	Equals = int16_t(Pass) << 1,
    PlusEquals = int16_t(Equals) << 1,
    MinusEquals = int16_t(PlusEquals) << 1,
    MultiplyEquals = int16_t(MinusEquals) << 1,
    DivideEquals = int16_t(MultiplyEquals) << 1
};

#define EXPECTED_PASS(Type, Flags, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<Type>(BehaviourFlags::Pass | Flags, Count))

unittest_util::CodeTests tests =
{
	// test an attribute type passes for all expression types
	{ "@a = (true);",               EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = max(1, 2);",            EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = 1 + i@b;",              EXPECTED_PASS(float, BehaviourFlags::Equals, 4) },
	{ "@a = -int@b;",               EXPECTED_PASS(float, BehaviourFlags::Equals, 3) },
	{ "@a = @b = 1;",               EXPECTED_PASS(float, BehaviourFlags::Equals, 4) },
	{ "@a = v@b.x = 1;",            EXPECTED_PASS(float, BehaviourFlags::Equals, 6) },
	{ "@a = ++float@b;",            EXPECTED_PASS(float, BehaviourFlags::Equals, 4) },
	{ "@a = bool(2);",              EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = {1, 2, 3};",            EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = v@b.x;",                EXPECTED_PASS(float, BehaviourFlags::Equals, 3) },
	{ "@a = 1s;",                   EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = \"a\";",                EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "@a = b;",                    EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },

	// test all attribute types pass for a single expression type with equals operator
	{ "bool@a = (true);",           EXPECTED_PASS(bool, BehaviourFlags::Equals, 2) },
	{ "short@a = (true);",          EXPECTED_PASS(short, BehaviourFlags::Equals, 2) },
	{ "i@a = (true);",              EXPECTED_PASS(int, BehaviourFlags::Equals, 2) },
	{ "int@a = (true);",            EXPECTED_PASS(int, BehaviourFlags::Equals, 2) },
	{ "long@a = (true);",           EXPECTED_PASS(long, BehaviourFlags::Equals, 2) },
	{ "f@a = (true);",              EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "float@a = (true);",          EXPECTED_PASS(float, BehaviourFlags::Equals, 2) },
	{ "double@a = (true);",         EXPECTED_PASS(double, BehaviourFlags::Equals, 2) },
	{ "vec3i@a = (true);",          EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::Equals, 2) },
	{ "v@a = (true);",              EXPECTED_PASS(openvdb::Vec3f, BehaviourFlags::Equals, 2) },
	{ "s@a = (true);",              EXPECTED_PASS(std::string, BehaviourFlags::Equals, 2) },
	{ "vec3f@a = (true);",          EXPECTED_PASS(openvdb::Vec3f, BehaviourFlags::Equals, 2) },
	{ "vec3d@a = (true);",          EXPECTED_PASS(openvdb::Vec3d, BehaviourFlags::Equals, 2) },
	{ "string@a = (true);",         EXPECTED_PASS(std::string, BehaviourFlags::Equals, 2) },

	// test plus equals operator
	{ "bool@a += (true);",          EXPECTED_PASS(bool, BehaviourFlags::PlusEquals, 4) },

	// test minus equals operator
	{ "bool@a -= (true);",          EXPECTED_PASS(bool, BehaviourFlags::MinusEquals, 4) },

	// test multiply equals operator
	{ "bool@a *= (true);",          EXPECTED_PASS(bool, BehaviourFlags::MultiplyEquals, 4) },

	// test divide equals operator
	{ "bool@a /= (true);",          EXPECTED_PASS(bool, BehaviourFlags::DivideEquals, 4) },

	// test component assignment
	{ "vec3i@a.x = (true);",        EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::Equals, 4) },
	{ "vec3i@a.y = (true);",        EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::Equals, 4) },
	{ "vec3i@a.z = (true);",        EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::Equals, 4) },
	{ "vec3i@a.x += (true);",       EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::PlusEquals, 6) },
	{ "vec3i@a.y += (true);",       EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::PlusEquals, 6) },
	{ "vec3i@a.z += (true);",       EXPECTED_PASS(openvdb::Vec3i, BehaviourFlags::PlusEquals, 6) },

	// test component assignment on all vector types
	{ "v@a.x = (true);",            EXPECTED_PASS(openvdb::Vec3f, BehaviourFlags::Equals, 4) },
	{ "vec3f@a.y = (true);",        EXPECTED_PASS(openvdb::Vec3f, BehaviourFlags::Equals, 4) },
	{ "vec3d@a.z = (true);",        EXPECTED_PASS(openvdb::Vec3d, BehaviourFlags::Equals, 4) },
};

struct AttributeAssignExpressionVisitor : public openvdb::ax::ast::Visitor
{
    AttributeAssignExpressionVisitor()
        : mCount(0)
        , mAssignExpressionNode(nullptr)
        , mBinaryOperatorNode(nullptr) {}

    ~AttributeAssignExpressionVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::AssignExpression& node) override final {
        ++mCount;
        mAssignExpressionNode = &node;
    }

    inline virtual void
    visit(const openvdb::ax::ast::BinaryOperator& node) override final {
        ++mCount;
        mBinaryOperatorNode = &node;
    };

    inline virtual void
    visit(const openvdb::ax::ast::Attribute& node) override final {
        ++mCount;
        mAttributeNode = &node;
    };

    size_t mCount;
    const openvdb::ax::ast::AssignExpression* mAssignExpressionNode;
    const openvdb::ax::ast::BinaryOperator* mBinaryOperatorNode;
    const openvdb::ax::ast::Attribute* mAttributeNode;
};

}

class TestAssignExpressionNode : public CppUnit::TestCase
{
 	public:

 	CPPUNIT_TEST_SUITE(TestAssignExpressionNode);
 	CPPUNIT_TEST(testSyntax);
 	CPPUNIT_TEST(testASTNode);
 	CPPUNIT_TEST_SUITE_END();

 	void testSyntax() { TEST_SYNTAX(tests); }
 	void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAssignExpressionNode);

void TestAssignExpressionNode::testASTNode()
{
	using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        AttributeAssignExpressionVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected AST node count", code),
            behaviour->count(), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
            static_cast<bool>(visitor.mAssignExpressionNode));

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected attribute type", code),
            behaviour->type(), visitor.mAttributeNode->mType);

        if (behaviour->hasFlag(BehaviourFlags::PlusEquals)) {
	        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
	            static_cast<bool>(visitor.mBinaryOperatorNode));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected += assign expression", code),
                visitor.mBinaryOperatorNode->mOperation, tokens::PLUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::MinusEquals)) {
	        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
	            static_cast<bool>(visitor.mBinaryOperatorNode));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected -= assign expression", code),
                visitor.mBinaryOperatorNode->mOperation, tokens::MINUS);
        }
        else if (behaviour->hasFlag(BehaviourFlags::MinusEquals)) {
	        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
	            static_cast<bool>(visitor.mBinaryOperatorNode));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected *= assign expression", code),
                visitor.mBinaryOperatorNode->mOperation, tokens::MULTIPLY);
        }
        else if (behaviour->hasFlag(BehaviourFlags::DivideEquals)) {
	        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code),
	            static_cast<bool>(visitor.mBinaryOperatorNode));
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected /= assign expression", code),
                visitor.mBinaryOperatorNode->mOperation, tokens::DIVIDE);
        }
    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
