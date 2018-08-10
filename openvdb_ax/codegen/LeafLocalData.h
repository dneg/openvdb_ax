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


#ifndef OPENVDB_AX_CODEGEN_LEAF_LOCAL_DATA_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_LEAF_LOCAL_DATA_HAS_BEEN_INCLUDED

#include <openvdb/openvdb.h>
#include <openvdb/points/AttributeArray.h>
#include <openvdb/points/PointAttribute.h>
#include <openvdb/points/PointDataGrid.h>
#include <openvdb/points/PointGroup.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {


/// @brief  Various functions can request the use and initialization of point data from within
///         the kernel that does not use the standard attribute handle methods. This data can
///         then be accessed after execution to perform post-processes such as adding new groups,
///         adding new string attributes or updating positions.
///
/// @note  Due to the way string handles work, string write attribute handles cannot
///        be constructed in parallel, nor can read handles retrieve values in parallel
///        if there is a chance the shared metadata is being written to (with set()).
///        As the compiler allows for any arbitrary string setting/getting, leaf local
///        maps are used for temporary storage per point. The maps use the string array
///        pointers as a key for later synchronization.
///
struct LeafLocalData
{
    using UniquePtr = std::unique_ptr<LeafLocalData>;
    using GroupArrayT = openvdb::points::GroupAttributeArray;
    using GroupHandleT = openvdb::points::GroupWriteHandle;

    using PointStringMap = std::map<uint64_t, std::string>;
    using StringArrayMap = std::map<points::AttributeArray*, PointStringMap>;

    using PositionT = openvdb::Vec3f;
    using PositionVector = std::vector<PositionT>;

    using LeafNode = openvdb::points::PointDataTree::LeafNodeType;

    /// @brief  Construct a new data object to keep track of various data objects
    ///         created per leaf by the point compute generator.
    ///
    /// @param  count  The number of points within the current leaf, used to initialize
    ///                the size of new arrays
    ///
    LeafLocalData(const size_t count)
        : mPointCount(count)
        , mArrays()
        , mOffset(0)
        , mHandles()
        , mStringMap()
        , mPositions() {}

    ////////////////////////////////////////////////////////////////////////

    /// Group methods

    /// @brief  Return a group write handle to a specific group name, creating the
    ///         group array if it doesn't exist. This includes either registering a
    ///         new offset or allocating an entire array. The returned handle is
    ///         guaranteed to be valid.
    ///
    /// @param  name  The group name
    ///
    inline GroupHandleT* getOrInsert(const std::string& name)
    {
        GroupHandleT* ptr = get(name);
        if (ptr) return ptr;

        static const size_t maxGroupsInArray =
            points::point_group_internal::GroupInfo::groupBits();

        if (mArrays.empty() || mOffset == maxGroupsInArray) {
            mArrays.emplace_back(new GroupArrayT(mPointCount));
            mOffset = 0;
        }

        GroupArrayT* array = mArrays.back().get();
        assert(array);

        std::unique_ptr<GroupHandleT>& handle = mHandles[name];
        handle.reset(new GroupHandleT(*array, mOffset++));
        return handle.get();
    }

    /// @brief  Return a group write handle to a specific group name if it exists.
    ///         Returns a nullptr if no group exists of the given name
    ///
    /// @param  name  The group name
    ///
    inline GroupHandleT* get(const std::string& name) const
    {
        const auto iter = mHandles.find(name);
        if (iter == mHandles.end()) return nullptr;
        return iter->second.get();
    }

    /// @brief  Return true if a valid group handle exists
    ///
    /// @param  name  The group name
    ///
    inline bool hasGroup(const std::string& name) const {
        return mHandles.find(name) != mHandles.end();
    }

    /// @brief  Populate a set with all the groups which have been inserted into
    ///         this object. Used to compute a final set of all new groups which
    ///         have been created across all leaf nodes
    ///
    /// @param  groups  The set to populate
    ///
    inline void getGroups(std::set<std::string>& groups) const {
        for (const auto& iter : mHandles) {
            groups.insert(iter.first);
        }
    }

    /// @brief  Compact all arrays stored on this object. This does not invalidate
    ///         any active write handles.
    ///
    inline void compact() {
        for (auto& array : mArrays) array->compact();
    }


    ////////////////////////////////////////////////////////////////////////

    /// String methods

    /// @brief  Get any new string data associated with a particular point on a
    ///         particular string attribute array. Returns true if data was set,
    ///         false if no data was found.
    ///
    /// @param  array  The array pointer to use as a key lookup
    /// @param  idx    The point index
    /// @param  data   The string to set if data is stored
    ///
    inline bool
    getNewStringData(const points::AttributeArray* array, const uint64_t idx, std::string& data) const {
        const auto arrayMapIter = mStringMap.find(const_cast<points::AttributeArray*>(array));
        if (arrayMapIter == mStringMap.end()) return false;
        const auto iter = arrayMapIter->second.find(idx);
        if (iter == arrayMapIter->second.end()) return false;
        data = iter->second;
        return true;
    }

    /// @brief  Set new string data associated with a particular point on a
    ///         particular string attribute array.
    ///
    /// @param  array  The array pointer to use as a key lookup
    /// @param  idx    The point index
    /// @param  data   The string to set
    ///
    inline void
    setNewStringData(points::AttributeArray* array, const uint64_t idx, const std::string& data) {
        mStringMap[array][idx] = data;
    }

    /// @brief  Remove any new string data associated with a particular point on a
    ///         particular string attribute array. Does nothing if no data exists
    ///
    /// @param  array  The array pointer to use as a key lookup
    /// @param  idx    The point index
    ///
    inline void
    removeNewStringData(points::AttributeArray* array, const uint64_t idx) {
        const auto arrayMapIter = mStringMap.find(array);
        if (arrayMapIter == mStringMap.end()) return;
        arrayMapIter->second.erase(idx);
        if (arrayMapIter->second.empty()) mStringMap.erase(arrayMapIter);
    }

    /// @brief  Insert all new point strings stored across all collected string
    ///         attribute arrays into a StringMetaInserter. Returns false if the
    ///         inserter was not accessed and true if it was potentially modified.
    ///
    /// @param  inserter  The string meta inserter to update
    ///
    inline bool
    insertNewStrings(points::StringMetaInserter& inserter) const {
        for (const auto& arrayIter : mStringMap) {
            for (const auto& iter : arrayIter.second) {
                inserter.insert(iter.second);
            }
        }
        return !mStringMap.empty();
    }

    /// @brief  Returns a const reference to the string array map
    ///
    inline const StringArrayMap& getStringArrayMap() const {
        return mStringMap;
    }


    ////////////////////////////////////////////////////////////////////////

    /// Position methods

    /// @brief Initialises the position vector for the all of the points in the leaf
    ///        that are included in the filter, other points have values of zero initialised

    /// @tparam FilterT    The filter type of the filter argument
    /// @param  leaf       The leaf node whose positions to cache
    /// @param  transform  The world-space transform of the grid
    /// @param  filter     A filter on the leaf to determine which points we need positions for

    template<typename FilterT = openvdb::points::NullFilter>
    inline void initPositions(const LeafNode& leaf, const openvdb::math::Transform& transform,
                              FilterT filter = FilterT()) {

        mPositions.reserve(mPointCount);

        const openvdb::points::AttributeSet& attributeSet = leaf.attributeSet();
        const size_t pos = attributeSet.find("P");
        assert(pos != openvdb::points::AttributeSet::INVALID_POS);

        const openvdb::points::AttributeHandle<openvdb::Vec3f>::Ptr
            position = openvdb::points::AttributeHandle<openvdb::Vec3f>::create(leaf.constAttributeArray(pos));

        filter.reset(leaf);

        for (auto voxel = leaf.cbeginValueAll(); voxel; ++voxel) {
            const openvdb::Coord& coord = voxel.getCoord();
            auto iter = leaf.beginIndexVoxel(coord);
            for (; iter; ++iter) {
                if (filter.valid(iter)) {
                    mPositions.emplace_back(transform.indexToWorld(coord.asVec3d() + position->get(*(iter))));
                }
                else mPositions.emplace_back(PositionT::zero());
            }
        }
    }

    /// @brief  Updates the position value in the the position vector
    ///
    /// @param  pos   The position to be assigned
    /// @param  index The index of the element the vector to assign to
    ///

    inline void setPosition(const PositionT& pos, const size_t index) {
        mPositions[index] = pos;
    }

    /// @brief  Returns a const reference to the position vector
    ///
    /// @note   The position vector must have been initialised
    ///

    inline const PositionVector& getPositions() const {
        return mPositions;
    }


private:

    const size_t mPointCount;
    std::vector<std::unique_ptr<GroupArrayT>> mArrays;
    points::GroupType mOffset;
    std::map<std::string, std::unique_ptr<GroupHandleT>> mHandles;
    StringArrayMap mStringMap;
    PositionVector mPositions;
};

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_LEAF_LOCAL_DATA_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
