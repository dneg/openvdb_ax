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

#include "PointFunctions.h"

#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/TargetRegistry.h>
#include <openvdb_ax/compiler/LeafLocalData.h>

#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>

namespace point_functions_internal
{

    /// @brief  Retrieve a group handle from an expected vector of handles using the offset
    ///         pointed to by the engine data. Note that HandleT should only ever be a GroupHandle
    ///         or GroupWriteHandle object
    template <typename HandleT>
    inline HandleT*
    groupHandle(const std::string& name, void** groupHandles, const void* const data)
    {
        const openvdb::points::AttributeSet* const attributeSet =
            static_cast<const openvdb::points::AttributeSet* const>(data);

        const size_t groupIdx = attributeSet->groupOffset(name);
        if (groupIdx == openvdb::points::AttributeSet::INVALID_POS) return nullptr;

        return static_cast<HandleT*>(groupHandles[groupIdx]);
    }

    void edit_group(const uint8_t* const name,
                   const uint64_t index,
                   void** groupHandles,
                   void* const leafDataPtr,
                   const void* const data,
                   const bool flag)
    {
        const char * const sarray = reinterpret_cast<const char* const>(name);
        const std::string nameStr(sarray);
        if (nameStr.empty()) return;

        openvdb::points::GroupWriteHandle* handle = nullptr;

        // Get the group handle out of the pre-existing container of handles if they
        // exist
        if (groupHandles) {
            handle = groupHandle<openvdb::points::GroupWriteHandle>(nameStr, groupHandles, data);
        }

        if (!handle) {
            openvdb::ax::compiler::LeafLocalData* const leafData =
                static_cast<openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

            // If we are setting membership and the handle doesnt exist, create in in
            // the set of new data thats being added
            if (!flag && !leafData->hasGroup(nameStr)) return;

            handle = leafData->getOrInsert(nameStr);
            assert(handle);
        }

        // set the group membership
        handle->set(index, flag);
    }

}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {


bool InGroup::Internal::in_group(const uint8_t* const name,
                                 const uint64_t index,
                                 void** groupHandles,
                                 const void* const leafDataPtr,
                                 const void* const data)
{
    if (!groupHandles) return false;

    const char * const sarray = reinterpret_cast<const char* const>(name);
    const std::string nameStr(sarray);
    if (nameStr.empty()) return false;

    const openvdb::points::GroupHandle* handle =
        point_functions_internal::groupHandle<openvdb::points::GroupHandle>(nameStr, groupHandles, data);
    if (handle) return handle->get(index);

    // If the handle doesn't exist, check to see if any new groups have
    // been added

    const openvdb::ax::compiler::LeafLocalData* const leafData =
        static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

    handle = leafData->get(nameStr);
    return handle ? handle->get(index) : false;
}

// size_t StringAttribSize::str_attrib_size(void* attributeHandle,
//                                          const uint64_t index,
//                                          const void* const leafDataPtr)
// {
//     using AttributeHandleType = openvdb::points::StringAttributeHandle;

//     assert(attributeHandle);
//     assert(leafDataPtr);

//     const AttributeHandleType* const handle =
//         static_cast<AttributeHandleType*>(attributeHandle);

//     const openvdb::ax::compiler::LeafLocalData* const leafData =
//         static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

//     std::string data;
//     if (!leafData->getNewStringData(&(handle->array()), index, data)) {
//         handle->get(data, index);
//     }

//     // include size of null terminator
//     return data.size() + 1;
// }

// void SetAttribute::set_attribute_string(void* attributeHandle,
//                                         const uint64_t index,
//                                         const uint8_t* value,
//                                         void* const leafDataPtr)
// {
//     using AttributeHandleType = openvdb::points::StringAttributeWriteHandle;

//     assert(attributeHandle);
//     assert(value);
//     assert(leafDataPtr);

//     const char* const sarray = reinterpret_cast<const char* const>(value);
//     const std::string s(sarray);

//     AttributeHandleType* const handle =
//         static_cast<AttributeHandleType*>(attributeHandle);

//     openvdb::ax::compiler::LeafLocalData* const leafData =
//         static_cast<openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

//     // Check to see if the string exists in the metadata cache. If so, set the string and
//     // remove any new data associated with it, otherwise set the new data

//     if (handle->hasIndex(s)) {
//         handle->set(index, s);
//         leafData->removeNewStringData(&(handle->array()), index);
//     }
//     else {
//         leafData->setNewStringData(&(handle->array()), index, s);
//     }
// }

void SetPointPWS::set_point_pws(void* leafDataPtr,
                                const uint64_t index,
                                openvdb::Vec3s* value)
{
    assert(index >= 0);
    openvdb::ax::compiler::LeafLocalData* leafData =
        static_cast<openvdb::ax::compiler::LeafLocalData*>(leafDataPtr);
    leafData->setPosition(*value, index);
}


// void GetAttribute::get_attribute_string(void* attributeHandle,
//                                       const uint64_t index,
//                                       uint8_t* value,
//                                       const void* const leafDataPtr)
// {
//     using AttributeHandleType = openvdb::points::StringAttributeHandle;

//     assert(attributeHandle);
//     assert(value);
//     assert(leafDataPtr);

//     AttributeHandleType* const handle =
//         static_cast<AttributeHandleType*>(attributeHandle);

//     const openvdb::ax::compiler::LeafLocalData* const leafData =
//         static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

//     std::string data;
//     if (!leafData->getNewStringData(&(handle->array()), index, data)) {
//         handle->get(data, index);
//     }

//     char* sarray = reinterpret_cast<char*>(value);
//     strcpy(sarray, data.c_str());
// }

void GetPointPWS::get_point_pws(void* leafDataPtr,
                                const uint64_t index,
                                openvdb::Vec3s* value)
{
    assert(index >= 0);
    openvdb::ax::compiler::LeafLocalData* leafData =
        static_cast<openvdb::ax::compiler::LeafLocalData*>(leafDataPtr);
   (*value) = leafData->getPositions()[index];
}


} // namespace codegen
} // namespace ax
} // namespace openvdb_version
} // namespace openvdb


// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
