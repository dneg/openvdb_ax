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
////////////////////////////////////////////////////////////////////////////

/// @file compiler/TargetRegistry.h
///
/// @authors Nick Avramoussis, Francisco Gochez
///
/// @brief These classes contain lists of expected attributes and volumes
///        which are populated by compiler during its internal code generation.
///        These will then be requested from the inputs to the executable
///        when execute is called. In this way, attributes are requested at
///        execution time, allowing the executable objects to be shared and
///        stored.
///
///
#ifndef OPENVDB_AX_COMPILER_TARGET_REGISTRY_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPILER_TARGET_REGISTRY_HAS_BEEN_INCLUDED

#include <openvdb/openvdb.h>
#include <openvdb/Types.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {

/// @brief This class stores a list of attribute names, types and whether they are
///        required to be writeable.
///
class AttributeRegistry
{
public:

    using Ptr = std::shared_ptr<AttributeRegistry>;
    using ConstPtr = std::shared_ptr<const AttributeRegistry>;

    /// @brief  Registered attribute details, including its name, type and whether
    ///         a write handle is required
    ///
    struct AttributeData
    {
        /// @brief Storage for attribute name, type and writeable details
        /// @param name      The name of the attribute
        /// @param type      The typename of the attribute
        /// @param writeable Whether the attribute needs to be writeable
        AttributeData(const Name& name, const Name& type, const bool writeable)
            : mName(name), mType(type), mWriteable(writeable) {}

        Name mName;
        Name mType;
        bool mWriteable;
    };

    using AttributeDataVec = std::vector<AttributeData>;

    AttributeRegistry()
        : mAttributes() {}

    /// @brief  Returns whether or not an attribute is required to be written to.
    ///         If no attribute with this name has been registered, returns false
    /// @param  name  The name of the attribute
    ///
    inline bool
    isAttributeWritable(const Name& name) const
    {
        for (const auto& data : mAttributes) {
            if (data.mName == name && data.mWriteable) {
                return true;
            }
        }
        return false;
    }

    /// @brief  Returns whether or not an attribute is registered.
    /// @param  name The name of the attribute
    ///
    inline bool
    isAttributeRegistered(const Name& name) const
    {
        for (const auto& data : mAttributes) {
            if (data.mName == name) return true;
        }
        return false;
    }

    /// @brief  Add an attribute to the registry, returns an index into
    ///         the registry for that attribute
    /// @param  name      The name of the attribute
    /// @param  type      The typename of the attribute
    /// @param  writeable Whether the attribute is required to be writeable
    ///
    inline int64_t
    addData(const Name& name, const Name& type, const bool writeable)
    {
        mAttributes.emplace_back(name, type, writeable);
        return mAttributes.size() - 1;
    }

    /// @brief  Returns a const reference to the vector of registered attributes
    ///
    inline const
    AttributeDataVec& attributeData() const
    {
        return mAttributes;
    }

private:
    AttributeDataVec mAttributes;
};


/// @brief This class stores a list of volume names, types and whether they are
///        required to be writeable
///
class VolumeRegistry
{
public:

    using Ptr = std::shared_ptr<VolumeRegistry>;
    using ConstPtr = std::shared_ptr<const VolumeRegistry>;

    /// @brief  Registered volume details, including its name, type and whether
    ///         a write accessor is required
    ///
    struct VolumeData
    {
        /// @brief Storage for volume name, type and writeable details
        /// @param name      The name of the volume
        /// @param type      The typename of the volume
        /// @param writeable Whether the volume needs to be writeable
        VolumeData(const Name& name, const Name& type, const bool writeable)
            : mName(name), mType(type), mWriteable(writeable) {}

        Name mName;
        Name mType;
        bool mWriteable;
    };

    using VolumeDataVec = std::vector<VolumeData>;

    VolumeRegistry()
        : mVolumes() {}

    /// @brief  Returns whether or not a volume is required to be written to.
    ///         If no volume with this name has been registered, returns false
    /// @param  name  The name of the volume
    ///
    inline bool
    isVolumeWriteable(const Name& name) const
    {
        for (const auto& data : mVolumes) {
            if (data.mName == name && data.mWriteable) {
                return true;
            }
        }
        return false;
    }

    /// @brief  Returns whether or not a volume is registered.
    /// @param  name The name of the attribute
    ///
    inline bool
    isVolumeRegistered(const Name& name) const
    {
        for (const auto& data : mVolumes) {
            if (data.mName == name) return true;
        }
        return false;
    }


    /// @brief  Add a volume to the registry, returns an index into
    ///         the registry for that volume
    /// @param  name      The name of the volume
    /// @param  type      The typename of the volume
    /// @param  writeable Whether the volume is required to be writeable
    ///
    inline int64_t
    addData(const Name& name, const Name& type, const bool writeable)
    {
        mVolumes.emplace_back(name, type, writeable);
        return mVolumes.size() - 1;
    }

    /// @brief  Returns a const reference to the vector of registered volumes
    ///
    inline const
    VolumeDataVec& volumeData() const
    {
        return mVolumes;
    }

private:
    VolumeDataVec mVolumes;
};


}
}
}

#endif // OPENVDB_AX_COMPILER_TARGET_REGISTRY_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
