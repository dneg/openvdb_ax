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

#include "TestHarness.h"

#include <openvdb/points/AttributeArray.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/points/PointGroup.h>

#include <cppunit/extensions/HelperMacros.h>

class TestGroups : public unittest_util::AXTestCase
{
public:
    CPPUNIT_TEST_SUITE(TestGroups);
    CPPUNIT_TEST(testAssignArithmeticToGroup);
    CPPUNIT_TEST(testGroupQuery);
    CPPUNIT_TEST(testGroupOrder);
    CPPUNIT_TEST_SUITE_END();

    void testAssignArithmeticToGroup();
    void testGroupQuery();
    void testGroupOrder();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestGroups);

void
TestGroups::testGroupOrder()
{
    // Test that groups inserted in a different alphabetical order are inferred
    // correctly (a regression test for a previous issue)

    mHarness.testVolumes(false);
    mHarness.addExpectedAttribute<int>("test", 1);
    mHarness.addInputGroups({"b", "a"}, {false, true});
    mHarness.addExpectedGroups({"b", "a"}, {false, true});

    mHarness.executeCode("test/snippets/assign/assignFromGroup");

    AXTESTS_STANDARD_ASSERT();
}

void
TestGroups::testAssignArithmeticToGroup()
{
    mHarness.testVolumes(false);
    const std::string groupName("group1");
    mHarness.addExpectedAttributes<float>({"float_test", "float_test2"}, {0.0f, 0.0f});
    mHarness.addInputGroups({groupName}, {false});
    mHarness.addExpectedGroups({groupName}, {false});

    mHarness.executeCode("test/snippets/assign/assignArithmetic", &groupName);

    AXTESTS_STANDARD_ASSERT();

    mHarness.reset();
    mHarness.addInputGroups({groupName}, {true});
    mHarness.addExpectedGroups({groupName}, {true});
    mHarness.addExpectedAttributes<float>({"float_test", "float_test2"}, {0.8125f, 2.0f});
    mHarness.executeCode("test/snippets/assign/assignArithmetic", &groupName);

    AXTESTS_STANDARD_ASSERT();
}

void
TestGroups::testGroupQuery()
{
    // test a tree with no groups
    openvdb::points::PointDataGrid::Ptr pointDataGrid1 = mHarness.mInputPointGrids["four_point"];
    openvdb::points::PointDataTree& pointTree = pointDataGrid1->tree();

    // compile and execute

    openvdb::ax::Compiler compiler;
    openvdb::ax::CustomData::Ptr customData = openvdb::ax::CustomData::create();
    std::string code = unittest_util::loadText( "test/snippets/function/functionInGroup");
    openvdb::ax::PointExecutable::Ptr executable =
        compiler.compile<openvdb::ax::PointExecutable>(code, customData);

    CPPUNIT_ASSERT_NO_THROW(executable->execute(*pointDataGrid1));

    // the snippet of code adds "groupTest" and groupTest2 attributes which should both have the values
    // "1" everywhere

    for (auto leafIter = pointTree.cbeginLeaf(); leafIter; ++leafIter) {
        openvdb::points::AttributeHandle<int> handle1(leafIter->attributeArray("groupTest"));
        openvdb::points::AttributeHandle<int> handle2(leafIter->attributeArray("groupTest2"));
        for (auto iter = leafIter->beginIndexAll(); iter; ++iter) {
            CPPUNIT_ASSERT_EQUAL(1, handle1.get(*iter));
            CPPUNIT_ASSERT_EQUAL(1, handle2.get(*iter));
        }
    }

    // there should be no groups - ensure none have been added by accident by query code
    auto leafIter = pointTree.cbeginLeaf();
    const openvdb::points::AttributeSet& attributeSet = leafIter->attributeSet();
    const openvdb::points::AttributeSet::Descriptor& descriptor1 = attributeSet.descriptor();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0),
                         static_cast<size_t>(descriptor1.groupMap().size()));

    // now we add a single group and run the test again
    openvdb::points::appendGroup(pointTree, "testGroup");
    setGroup(pointTree, "testGroup", false);

    executable = compiler.compile<openvdb::ax::PointExecutable>(code, customData);
    CPPUNIT_ASSERT_NO_THROW(executable->execute(*pointDataGrid1));

    for (auto leafIter = pointTree.cbeginLeaf(); leafIter; ++leafIter) {
        openvdb::points::AttributeHandle<int> handle1(leafIter->attributeArray("groupTest"));
        openvdb::points::AttributeHandle<int> handle2(leafIter->attributeArray("groupTest2"));
        for (auto iter = leafIter->beginIndexAll(); iter; ++iter) {
            CPPUNIT_ASSERT_EQUAL(1, handle1.get(*iter));
            CPPUNIT_ASSERT_EQUAL(1, handle2.get(*iter));
        }
    }

    // for the next couple of tests we create a small tree with 4 points.  We wish to test queries of a single group
    // in a tree that has several groups
    std::vector<openvdb::math::Vec3s> positions =  {
                                        {1, 1, 1},
                                        {1, 2, 1},
                                        {2, 1, 1},
                                        {2, 2, 1},
                                    };

    const float voxelSize(1.0);
    openvdb::math::Transform::Ptr transform(openvdb::math::Transform::createLinearTransform(voxelSize));
    const openvdb::points::PointAttributeVector<openvdb::math::Vec3s> pointList(positions);
    openvdb::tools::PointIndexGrid::Ptr pointIndexGrid =
            openvdb::tools::createPointIndexGrid<openvdb::tools::PointIndexGrid>(
                pointList, *transform);
    openvdb::points::PointDataGrid::Ptr pointDataGrid2 =  openvdb::points::createPointDataGrid<openvdb::points::NullCodec, openvdb::points::PointDataGrid>(
                *pointIndexGrid, pointList, *transform);
    openvdb::points::PointDataTree::Ptr pointDataTree2 = pointDataGrid2->treePtr();

    // add 9 groups.  8 groups can be added by using a single group attribute, but this requires adding another attribute
    // and hence exercises the code better
    for(size_t i = 0; i < 9; i++) {
        std::stringstream newGroupNameStream;
        newGroupNameStream << "testGroup" << i;
        openvdb::points::appendGroup(*pointDataTree2, newGroupNameStream.str());
    }
    std::vector<short> membershipTestGroup2{0, 0, 1, 0};
    openvdb::points::setGroup(*pointDataTree2, pointIndexGrid->tree(), membershipTestGroup2, "testGroup2");

    customData->reset();
    executable = compiler.compile<openvdb::ax::PointExecutable>(code, customData);
    CPPUNIT_ASSERT_NO_THROW(executable->execute(*pointDataGrid2));

    auto leafIter2 = pointDataTree2->cbeginLeaf();
    const openvdb::points::AttributeSet& attributeSet2 = leafIter2->attributeSet();
    openvdb::points::AttributeHandle<int> testResultAttributeHandle(*attributeSet2.get("groupTest2"));

    // these should line up with the defined membership
    CPPUNIT_ASSERT_EQUAL(testResultAttributeHandle.get(0), 1);
    CPPUNIT_ASSERT_EQUAL(testResultAttributeHandle.get(1), 1);
    CPPUNIT_ASSERT_EQUAL(testResultAttributeHandle.get(2), 2);
    CPPUNIT_ASSERT_EQUAL(testResultAttributeHandle.get(3), 1);

    // check that no new groups have been created or deleted
    const openvdb::points::AttributeSet::Descriptor& descriptor2 = attributeSet2.descriptor();

    CPPUNIT_ASSERT_EQUAL(static_cast<openvdb::points::AttributeSet::Descriptor::NameToPosMap::size_type>(9),
                         descriptor2.groupMap().size());

    for(size_t i = 0; i < 9; i++) {
        std::stringstream newGroupNameStream;
        newGroupNameStream << "testGroup" << i;
        CPPUNIT_ASSERT(descriptor2.hasGroup(newGroupNameStream.str()));
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
