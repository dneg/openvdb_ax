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

#include "CompareGrids.h"
#include "TestHarness.h"

#include <openvdb_ax/test/util.h>

#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/Exceptions.h>

#include <cppunit/extensions/HelperMacros.h>

using namespace openvdb::points;

class TestAssign : public unittest_util::AXTestCase
{
public:

    CPPUNIT_TEST_SUITE(TestAssign);
    CPPUNIT_TEST(testAssignArithmeticPoints);
    CPPUNIT_TEST(testAssignArithmeticVolumes);
    CPPUNIT_TEST(testAssignChains);
    CPPUNIT_TEST(testAssignDecrementArithmetic);
    CPPUNIT_TEST(testAssignExpression);
    CPPUNIT_TEST(testAssignIncrementArithmetic);
    CPPUNIT_TEST(testAssignLocalVariables);
    CPPUNIT_TEST(testAssignLocalVectorVariableElements);
    CPPUNIT_TEST(testAssignLocalVectorVariables);
    CPPUNIT_TEST(testAssignScalarAttributes);
    CPPUNIT_TEST(testAssignVectorAttributeElements);
    CPPUNIT_TEST(testAssignVectorAttributes);
    CPPUNIT_TEST(testAssignVectorMappings);
    CPPUNIT_TEST(testAssignVectorFromScalar);
    CPPUNIT_TEST(testAssignScalarFromVector);
    CPPUNIT_TEST(testAssignScopedLocalVariables);
    CPPUNIT_TEST(testAssignDuplicateLocalVariables);
    CPPUNIT_TEST(testAssignDuplicateScopedLocalVariables);
    CPPUNIT_TEST(testAssignFromExternals);
    CPPUNIT_TEST_SUITE_END();

    void testAssignArithmeticPoints();
    void testAssignArithmeticVolumes();
    void testAssignChains();
    void testAssignDecrementArithmetic();
    void testAssignExpression();
    void testAssignIncrementArithmetic();
    void testAssignLocalVariables();
    void testAssignLocalVectorVariableElements();
    void testAssignLocalVectorVariables();
    void testAssignScalarAttributes();
    void testAssignVectorAttributeElements();
    void testAssignVectorAttributes();
    void testAssignVectorMappings();
    void testAssignVectorFromScalar();
    void testAssignScalarFromVector();
    void testAssignScopedLocalVariables();
    void testAssignDuplicateLocalVariables();
    void testAssignDuplicateScopedLocalVariables();
    void testAssignFromExternals();

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestAssign);

void
TestAssign::testAssignArithmeticPoints()
{
    mHarness.testVolumes(false);
    mHarness.addAttributes<float>({"float_test", "float_test2"}, {0.8125f, 2.0f});

    mHarness.executeCode("test/snippets/assign/assignArithmetic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignArithmeticVolumes()
{
    openvdb::math::Transform::Ptr transform1 = openvdb::math::Transform::createLinearTransform(0.1);
    transform1->postTranslate(openvdb::math::Vec3d(0.1,0,0));
    openvdb::FloatGrid::Ptr float_test = openvdb::FloatGrid::create();
    float_test->setTransform(transform1);
    float_test->setName("float_test");
    float_test->tree().setValueOn(openvdb::Coord(0));
    float_test->tree().setValueOn(openvdb::Coord(-1, 0, 0));

    openvdb::math::Transform::Ptr transform2 = openvdb::math::Transform::createLinearTransform(0.1);
    openvdb::FloatGrid::Ptr float_test2 = openvdb::FloatGrid::create();
    float_test2->setTransform(transform2);
    float_test2->tree().setValueOn(openvdb::Coord(0));
    float_test2->setName("float_test2");

    openvdb::GridPtrVec grids;
    grids.push_back(float_test);
    grids.push_back(float_test2);

    CPPUNIT_ASSERT_NO_THROW(unittest_util::wrapExecution(grids, "test/snippets/assign/assignArithmetic"));

    CPPUNIT_ASSERT_EQUAL(0.8125f, float_test->tree().getValue(openvdb::Coord(-1,0,0)));
    CPPUNIT_ASSERT_EQUAL(0.5625f, float_test->tree().getValue(openvdb::Coord(0)));
    CPPUNIT_ASSERT_EQUAL(2.0f, float_test2->tree().getValue(openvdb::Coord(0)));
}

void
TestAssign::testAssignChains()
{
    // NOTE: Chained assignments don't work correctly for volume execution, so this test cannot be
    // run for volumes
    mHarness.testVolumes(false);
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 6),
        {0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f});

    std::vector<openvdb::Vec3f> vecAttributeValues{openvdb::Vec3f(0.f, 0.f, 0.f),
        openvdb::Vec3f(0.f, 0.f, 0.f), openvdb::Vec3f(0.f, 0.f, 0.f), openvdb::Vec3f(0.f, -3.f, 0.f),
        openvdb::Vec3f(0.f, -3.f, 0.f), openvdb::Vec3f(0.f, -3.f, 0.0f)};

    mHarness.addAttributes(unittest_util::nameSequence("vec_float_test", 6),
        vecAttributeValues);

    mHarness.executeCode("test/snippets/assign/assignChains");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignDecrementArithmetic()
{
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 4),
        {-0.5f, -0.5f, 0.5f, -0.5});
    mHarness.executeCode("test/snippets/assign/assignDecrementArithmetic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignExpression()
{
    // NOTE: Chained assignments don't work for volume execution, so this test cannot be run for
    // volumes
    mHarness.testVolumes(false);
    mHarness.addAttributes<float>({"float_test", "float_test2"}, {2.5f, 2.5f});
    mHarness.executeCode("test/snippets/assign/assignExpression");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignIncrementArithmetic()
{
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 4),
        {1.5f, 1.5f, 0.5f, 1.5f});

    mHarness.executeCode("test/snippets/assign/assignIncrementArithmetic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignLocalVariables()
{
    mHarness.addAttributes<float>({"float_test"}, {0.8f});
    mHarness.executeCode("test/snippets/assign/assignLocalVariables");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignLocalVectorVariableElements()
{
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test"}, {openvdb::Vec3f(5.f, 4.f, 3.f)});
    mHarness.executeCode("test/snippets/assign/assignLocalVectorVariableElements");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignLocalVectorVariables()
{
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test"}, {openvdb::Vec3f(1.f, 2.f, 3.f)});
    mHarness.executeCode("test/snippets/assign/assignLocalVectorVariables");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignScalarAttributes()
{
    mHarness.testVolumes(false);
    mHarness.addExpectedAttributes<float>({"float_test", "float_test2"}, {0.5f, 0.5f});
    mHarness.executeCode("test/snippets/assign/assignScalarAttributes");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignVectorAttributeElements()
{
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test"}, {openvdb::Vec3f(5.f, 4.f, 3.f)});
    mHarness.executeCode("test/snippets/assign/assignVectorAttributeElements");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignVectorAttributes()
{
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test", "vec_float_test2"},
        {openvdb::Vec3f(1.f, 2.f, 3.f), openvdb::Vec3f(1.f, 2.f, 3.f)});
    mHarness.executeCode("test/snippets/assign/assignVectorAttributes");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignVectorMappings()
{
    mHarness.testVolumes(false);
    const std::vector<openvdb::Vec3f> values{openvdb::Vec3f(0.1f, 0.2f, 0.8f),
        openvdb::Vec3f(0.2f, 0.3f, 0.6f), openvdb::Vec3f(200.1f, 100.2f, 55.2f)};

    mHarness.addExpectedAttributes<openvdb::Vec3f>({"N", "Cd", "v"}, values);
    mHarness.executeCode("test/snippets/assign/assignVectorMappings");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignVectorFromScalar()
{
    mHarness.addAttributes<float>({"float_test1"}, {10.f});
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test1", "vec_float_test2", "vec_float_test3"},
        {openvdb::Vec3f(10.f, 10.f, 10.f), openvdb::Vec3f(-100.f, -100.f, -100.f),
         openvdb::Vec3f(-3.f, -3.f, -3.f)});

    mHarness.executeCode("test/snippets/assign/assignVectorFromScalar");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignScalarFromVector()
{
    mHarness.addAttributes<float>({"float_test1", "float_test2", "float_test3"},
        {10.0f, -10.0f, -5.0f});

    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test1"}, {openvdb::Vec3f(10.f, 9.f, 8.f)});
    mHarness.executeCode("test/snippets/assign/assignScalarFromVector");

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignScopedLocalVariables()
{
    const std::vector<std::string> floatNames{"float_test", "float_test2", "float_test3",
        "float_test4", "float_test5", "float_test6", "float_test7"};
    const std::vector<float> floatValues{30.0f, 1.0f, -10.0f, -15.0f, 50.0f, 50.0f, 1.0f};

    mHarness.addAttributes(floatNames, floatValues);

    std::vector<std::string> warnings;
    mHarness.executeCode("test/snippets/assign/assignScopedLocalVariables", nullptr, &warnings);

    CPPUNIT_ASSERT(!warnings.empty());

    AXTESTS_STANDARD_ASSERT();
}

void
TestAssign::testAssignDuplicateLocalVariables()
{
    CPPUNIT_ASSERT_THROW(
        mHarness.executeCode("test/snippets/assign/assignDuplicateLocalVariables"),
            openvdb::LLVMDeclarationError
    );
}

void
TestAssign::testAssignDuplicateScopedLocalVariables()
{
    CPPUNIT_ASSERT_THROW(
        mHarness.executeCode("test/snippets/assign/assignDuplicateScopedLocalVariables"),
            openvdb::LLVMDeclarationError
        );
}

void
TestAssign::testAssignFromExternals()
{
    mHarness.addAttribute<int32_t>("int_test1", -3);
    mHarness.addAttribute<int32_t>("int_test2", 1);
    mHarness.addAttribute<int64_t>("long_test1", 2l);
    mHarness.addAttribute<float>("float_test1", 5.6f);
    mHarness.addAttribute<float>("float_test2", 8.3f);
    mHarness.addAttribute<float>("float_test3", -1.0f, 0.0f); // float_test3 should be set to zero from empty data
    mHarness.addAttribute<double>("double_test1", 3.3);
    mHarness.addAttribute<openvdb::Vec3f>("vector_test1", openvdb::Vec3f(5.f, 4.f, 3.f));
    mHarness.addAttribute<openvdb::Vec3f>("vector_test2", openvdb::Vec3f(1.f, 2.f, 6.f));
    mHarness.addAttribute<openvdb::Vec3i>("vector_test3", openvdb::Vec3i(10, 11, 12));
    mHarness.addAttribute<openvdb::Vec3d>("vector_test4", openvdb::Vec3d(4.5, 4.4, 4.3));

    openvdb::ax::CustomData::Ptr data = openvdb::ax::CustomData::create();
    data->insertData("short_test1", openvdb::TypedMetadata<int16_t>(-3).copy());
    data->insertData("int_test2", openvdb::TypedMetadata<int32_t>(1).copy());
    data->insertData("long_test1", openvdb::TypedMetadata<int64_t>(2l).copy());
    data->insertData("float_test1", openvdb::TypedMetadata<float>(5.6f).copy());
    data->insertData("float_test2", openvdb::TypedMetadata<float>(8.3f).copy());
    data->insertData("double_test1", openvdb::TypedMetadata<double>(3.3).copy());
    data->insertData("vector_test1", openvdb::TypedMetadata<openvdb::Vec3f>(openvdb::Vec3f(5.f, 4.f, 3.f)).copy());
    data->insertData("vector_test2", openvdb::TypedMetadata<openvdb::Vec3f>(openvdb::Vec3f(1.f, 2.f, 6.f)).copy());
    data->insertData("vector_test3", openvdb::TypedMetadata<openvdb::Vec3i>(openvdb::Vec3i(10, 11, 12)).copy());
    data->insertData("vector_test4", openvdb::TypedMetadata<openvdb::Vec3d>(openvdb::Vec3d(4.5, 4.4, 4.3)).copy());

    mHarness.executeCode("test/snippets/assign/assignFromExternals", nullptr, nullptr, data);

    AXTESTS_STANDARD_ASSERT();
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
