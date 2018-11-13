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

#define EXPECTED_PASS(Type) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<Type>(unittest_util::ExpectedBase::Pass))

static const unittest_util::CodeTests tests =
{
    { "$a;",       EXPECTED_PASS(float) },
    { "bool$a;",   EXPECTED_PASS(bool) },
    { "short$a;",  EXPECTED_PASS(short) },
    { "i$a;",      EXPECTED_PASS(int) },
    { "int$a;",    EXPECTED_PASS(int) },
    { "long$a;",   EXPECTED_PASS(long) },
    { "f$a;",      EXPECTED_PASS(float) },
    { "float$a;",  EXPECTED_PASS(float) },
    { "double$a;", EXPECTED_PASS(double) },
    { "vec3i$a;",  EXPECTED_PASS(openvdb::Vec3i) },
    { "v$a;",      EXPECTED_PASS(openvdb::Vec3f) },
    { "s$a;",      EXPECTED_PASS(std::string) },
    { "vec3f$a;",  EXPECTED_PASS(openvdb::Vec3f) },
    { "vec3d$a;",  EXPECTED_PASS(openvdb::Vec3d) },
    { "string$a;", EXPECTED_PASS(std::string) },
};

struct ExternalVariableVisitor : public openvdb::ax::ast::Visitor
{
    ExternalVariableVisitor()
        : mCount(0)
        , mExternalVariable(nullptr) {}
    ~ExternalVariableVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::ExternalVariable& node) override final {
        ++mCount;
        mExternalVariable = &node;
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::ExternalVariable* mExternalVariable;
};

}

class TestExternalVariableNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestExternalVariableNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestExternalVariableNode);

void
TestExternalVariableNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (behaviour->fails()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        ExternalVariableVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected AST node count", code),
            behaviour->count(), visitor.mCount);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid AST External Variable node", code),
            static_cast<bool>(visitor.mExternalVariable));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected External Variable type", code),
            behaviour->type(), visitor.mExternalVariable->mType);
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
