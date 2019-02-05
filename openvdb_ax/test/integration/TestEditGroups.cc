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

#include "TestHarness.h"

#include <openvdb/points/AttributeArray.h>
#include <openvdb/points/PointConversion.h>
#include <openvdb/points/PointGroup.h>

#include <cppunit/extensions/HelperMacros.h>

class TestEditGroups : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(TestEditGroups);
    CPPUNIT_TEST(testAddToRemoveFromGroup);
    CPPUNIT_TEST_SUITE_END();

    void testAddToRemoveFromGroup();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestEditGroups);


// tests both the addition and removal of group membership
void
TestEditGroups::testAddToRemoveFromGroup()
{
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

    openvdb::points::PointDataGrid::Ptr dataGrid =
        openvdb::points::createPointDataGrid<openvdb::points::NullCodec, openvdb::points::PointDataGrid>(
                *pointIndexGrid, pointList, *transform);

    openvdb::points::PointDataTree& dataTree = dataGrid->tree();

    // apppend a new attribute for stress testing

    openvdb::points::appendAttribute(dataTree, "existingTestAttribute", 2);
    openvdb::points::appendGroup(dataTree, "existingTestGroup");

    std::vector<short> membershipTestGroup1{1, 0, 1, 0};
    openvdb::points::setGroup(dataTree, pointIndexGrid->tree(), membershipTestGroup1, "existingTestGroup");

    // second pre-existing group.
    openvdb::points::appendGroup(dataTree, "existingTestGroup2");
    openvdb::points::setGroup(dataTree, "existingTestGroup2", false);

    unittest_util::wrapExecution(*dataGrid, "test/snippets/function/functionAddRemoveFromGroup");

    auto leafIter = dataTree.cbeginLeaf();

    const openvdb::points::AttributeSet& attributeSet = leafIter->attributeSet();
    const openvdb::points::AttributeSet::Descriptor& desc = attributeSet.descriptor();

    for (size_t i = 1; i <= 9; i++) {
        std::stringstream groupNameStream;
        groupNameStream <<  "newTestGroup" << i;
        CPPUNIT_ASSERT_MESSAGE(groupNameStream.str() + " doesn't exist", desc.hasGroup(groupNameStream.str()));
    }
    openvdb::points::GroupHandle newTestGroupHandle = leafIter->groupHandle("newTestGroup9");
    CPPUNIT_ASSERT(!newTestGroupHandle.get(0));
    CPPUNIT_ASSERT(newTestGroupHandle.get(1));
    CPPUNIT_ASSERT(!newTestGroupHandle.get(2));
    CPPUNIT_ASSERT(newTestGroupHandle.get(3));

    // other new groups should be untouched
    for (size_t i = 1; i <= 8; i++) {
        std::stringstream groupNameStream;
        groupNameStream <<  "newTestGroup" << i;
        openvdb::points::GroupHandle handle = leafIter->groupHandle(groupNameStream.str());
        CPPUNIT_ASSERT(handle.get(0));
        CPPUNIT_ASSERT(handle.get(1));
        CPPUNIT_ASSERT(handle.get(2));
        CPPUNIT_ASSERT(handle.get(3));
    }

    openvdb::points::GroupHandle existingTestGroupHandle = leafIter->groupHandle("existingTestGroup");
    CPPUNIT_ASSERT(existingTestGroupHandle.get(0));
    CPPUNIT_ASSERT(!existingTestGroupHandle.get(1));
    CPPUNIT_ASSERT(existingTestGroupHandle.get(2));
    CPPUNIT_ASSERT(!existingTestGroupHandle.get(3));

    // membership of this group should now mirror exisingTestGroup
    openvdb::points::GroupHandle existingTestGroup2Handle = leafIter->groupHandle("existingTestGroup2");
    CPPUNIT_ASSERT(existingTestGroup2Handle.get(0));
    CPPUNIT_ASSERT(!existingTestGroup2Handle.get(1));
    CPPUNIT_ASSERT(existingTestGroup2Handle.get(2));
    CPPUNIT_ASSERT(!existingTestGroup2Handle.get(3));

    // check that "nonExistentGroup" was _not_ added to the tree, as it is removed from but not present
    CPPUNIT_ASSERT(!desc.hasGroup("nonExistentGroup"));

    // now check 2 new attributes added to tree
    openvdb::points::AttributeHandle<int> testResultAttributeHandle1(*attributeSet.get("newTestAttribute1"));
    openvdb::points::AttributeHandle<int> testResultAttributeHandle2(*attributeSet.get("newTestAttribute2"));
    for (size_t i = 0;i < 4; i++) {
        CPPUNIT_ASSERT(testResultAttributeHandle1.get(i));
    }

    // should match "existingTestGroup"
    CPPUNIT_ASSERT(testResultAttributeHandle2.get(0));
    CPPUNIT_ASSERT(!testResultAttributeHandle2.get(1));
    CPPUNIT_ASSERT(testResultAttributeHandle2.get(2));
    CPPUNIT_ASSERT(!testResultAttributeHandle2.get(3));

    // pre-existing attribute should still be present with the correct value

    for (; leafIter; ++leafIter) {
        openvdb::points::AttributeHandle<int> handle(leafIter->attributeArray("existingTestAttribute"));
        CPPUNIT_ASSERT(handle.isUniform());
        CPPUNIT_ASSERT_EQUAL(2, handle.get(0));
    }
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
