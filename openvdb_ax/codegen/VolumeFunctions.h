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

/// @file codegen/VolumeFunctions.h
///
/// @authors Nick Avramoussis, Richard Jones
///
/// @brief  Contains the function objects that define the functions used in
///         volume compute function generation, to be inserted into the FunctionRegistry.
///         These define the functions available when operating on volumes.
///         Also includes the definitions for the volume value retrieval and setting.
///
///

#ifndef OPENVDB_AX_CODEGEN_VOLUME_FUNCTIONS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_VOLUME_FUNCTIONS_HAS_BEEN_INCLUDED

#include "Functions.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/compiler/CompilerOptions.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/Exceptions.h>

#include <openvdb/version.h>

#include <unordered_map>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

struct GetVoxelPWS : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getvoxelpws", FunctionBase::Volume,
        "Returns the current voxel's position in world space as a vector float.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetVoxelPWS()); }

    GetVoxelPWS() : FunctionBase({
        FunctionSignature<V3F*()>::create(nullptr, std::string("getvoxelpws"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {
        return globals.at("coord_ws");
    }
};

struct GetCoordX : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getcoordx", FunctionBase::Volume,
        "Returns the current voxel's X index value in index space as an integer.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetCoordX()); }

    GetCoordX() : FunctionBase({
        FunctionSignature<int()>::create(nullptr, std::string("getcoordx"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {
        return builder.CreateLoad(builder.CreateConstGEP2_64(globals.at("coord_is"), 0, 0));
    }
};

struct GetCoordY : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getcoordy", FunctionBase::Volume,
        "Returns the current voxel's Y index value in index space as an integer.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetCoordY()); }

    GetCoordY() : FunctionBase({
        FunctionSignature<int()>::create(nullptr, std::string("getcoordy"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {
        return builder.CreateLoad(builder.CreateConstGEP2_64(globals.at("coord_is"), 0, 1));
    }
};

struct GetCoordZ : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getcoordz", FunctionBase::Volume,
        "Returns the current voxel's Z index value in index space as an integer.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetCoordZ()); }

    GetCoordZ() : FunctionBase({
        FunctionSignature<int()>::create(nullptr, std::string("getcoordz"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {
        return builder.CreateLoad(builder.CreateConstGEP2_64(globals.at("coord_is"), 0, 2));
    }
};

struct SetVoxel : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("setvoxel", FunctionBase::Volume,
        "Internal function for setting the value of a voxel.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new SetVoxel()); }

    SetVoxel() : FunctionBase({
        // pod types pass by value
        DECLARE_FUNCTION_SIGNATURE(set_voxel<double>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel<float>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel<int64_t>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel<int32_t>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel<int16_t>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel<bool>),
        // non-pod types pass by ptr
        DECLARE_FUNCTION_SIGNATURE(set_voxel_ptr<openvdb::Vec3d>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel_ptr<openvdb::Vec3f>),
        DECLARE_FUNCTION_SIGNATURE(set_voxel_ptr<openvdb::Vec3i>)
    }) {}

private:
    template <typename ValueT>
    inline static void set_voxel_ptr(void* accessor, const int32_t (*coord)[3], const ValueT* value)
    {
        using GridType = typename openvdb::BoolGrid::ValueConverter<ValueT>::Type;
        using AccessorType = typename GridType::Accessor;

        assert(accessor);
        assert(coord);

        AccessorType* const accessorPtr = static_cast<AccessorType* const>(accessor);

        // Currently set value only to avoid changing topology
        accessorPtr->setValueOnly(openvdb::Coord(coord[0]), *value);
    }

    template <typename ValueT>
    inline static void set_voxel(void* accessor, const int32_t (*coord)[3], const ValueT value)
    {
        set_voxel_ptr<ValueT>(accessor, coord, &value);
    }

};

struct GetVoxel : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getvoxel", FunctionBase::Volume,
        "Internal function for getting the value of a voxel.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetVoxel()); }

    GetVoxel() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<double>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<float>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<int64_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<int32_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<int16_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<bool>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<openvdb::Vec3d>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<openvdb::Vec3f>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_voxel<openvdb::Vec3i>, 1)

    }) {}

private:
    template <typename ValueT>
    inline static void get_voxel(void* accessor, void* transform, const float (*coord)[3], ValueT* value)
    {
        using GridType = typename openvdb::BoolGrid::ValueConverter<ValueT>::Type;
        using AccessorType = typename GridType::Accessor;

        assert(accessor);
        assert(coord);
        assert(transform);

        const AccessorType* const accessorPtr = static_cast<const AccessorType* const>(accessor);
        const openvdb::math::Transform* const transformPtr =
                static_cast<const openvdb::math::Transform* const>(transform);
        openvdb::Vec3d coordWS(*coord);
        openvdb::Coord coordIS = transformPtr->worldToIndexCellCentered(coordWS);

        (*value) = accessorPtr->getValue(coordIS);
    }

};

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_VOLUME_FUNCTIONS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
