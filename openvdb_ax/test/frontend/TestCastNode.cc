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

#define EXPECTED_PASS(Type, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<Type>(unittest_util::ExpectedBase::Pass, Count))

static const unittest_util::CodeTests tests =
{
    { "bool(a);",               EXPECTED_PASS(bool, 1) },
    { "short(a);",              EXPECTED_PASS(short, 1) },
    { "int(a);",                EXPECTED_PASS(int, 1) },
    { "long(a);",               EXPECTED_PASS(long, 1) },
    { "float(a);",              EXPECTED_PASS(float, 1) },
    { "double(a);",             EXPECTED_PASS(double, 1) },
    { "float(double(int(0)));", EXPECTED_PASS(float, 3) },
    { "int(0);",                EXPECTED_PASS(int, 1) },
    { "int(@a);",               EXPECTED_PASS(int, 1) },
    { "int((a));",              EXPECTED_PASS(int, 1) },
    { "int(function());",       EXPECTED_PASS(int, 1) },
    { "bool(a+a);",             EXPECTED_PASS(bool, 1) },
    { "short(a+a);",            EXPECTED_PASS(short, 1) },
    { "int(~a);",               EXPECTED_PASS(int, 1) },
    { "long(~a);",              EXPECTED_PASS(long, 1) },
    { "float(a = a);",          EXPECTED_PASS(float, 1) },
    { "double(a.x = 0);",       EXPECTED_PASS(double, 1) },
    { "int(a++);",              EXPECTED_PASS(int, 1) },
    { "int({a,b,c});",          EXPECTED_PASS(int, 1) },
    { "bool({a,b,c});",         EXPECTED_PASS(bool, 1) },
    { "double(true);",          EXPECTED_PASS(double, 1) },
    { "double(false);",         EXPECTED_PASS(double, 1) },
    { "short(a.x);",            EXPECTED_PASS(short, 1) },
    { "int(1.0f);",             EXPECTED_PASS(int, 1) },
    { "long(1.0);",             EXPECTED_PASS(long, 1) },
    { "float(true);",           EXPECTED_PASS(float, 1) },
    { "double(1s);",            EXPECTED_PASS(double, 1) },
    { "int(1l);",               EXPECTED_PASS(int, 1) },
    { "int(1);",                EXPECTED_PASS(int, 1) },
};

struct CastVisitor : public openvdb::ax::ast::Visitor
{
    CastVisitor()
        : mCount(0), mNode(nullptr) {}
    ~CastVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::Cast& node) override final {
        ++mCount;
        mNode = &node;
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::Cast* mNode;
};

}

class TestCastNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestCastNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCastNode);

void
TestCastNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (!behaviour->passes()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        CastVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            behaviour->count(), visitor.mCount);

        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node", code), visitor.mNode);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST node Expression", code), visitor.mNode->mExpression);

        if (behaviour->isType<bool>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected bool cast type.", code),
                std::string(openvdb::typeNameAsString<bool>()), visitor.mNode->mType);

        }
        else if (behaviour->isType<int16_t>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected int16_t cast type.", code),
                std::string(openvdb::typeNameAsString<int16_t>()), visitor.mNode->mType);

        }
        else if (behaviour->isType<int32_t>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected int32_t cast type.", code),
                std::string(openvdb::typeNameAsString<int32_t>()), visitor.mNode->mType);

        }
        else if (behaviour->isType<int64_t>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected int64_t cast type.", code),
                std::string(openvdb::typeNameAsString<int64_t>()), visitor.mNode->mType);

        }
        else if (behaviour->isType<float>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected float cast type.", code),
                std::string(openvdb::typeNameAsString<float>()), visitor.mNode->mType);

        }
        else if (behaviour->isType<double>()) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected double cast type.", code),
                std::string(openvdb::typeNameAsString<double>()), visitor.mNode->mType);

        }
        else {
            CPPUNIT_FAIL(ERROR_MSG("Unsupported cast type.", code));
        }
    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
