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

/// @file codegen/PointFunctions.h
///
/// @authors Nick Avramoussis, Richard Jones
///
/// @brief  Contains the function objects that define the functions used in
///         point compute function generation, to be inserted into the FunctionRegistry.
///         These define the functions available when operating on points.
///         Also includes the definitions for the point attribute retrieval
///         and setting.
///
///

#ifndef OPENVDB_AX_CODEGEN_POINT_FUNCTIONS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_POINT_FUNCTIONS_HAS_BEEN_INCLUDED

#include "FunctionTypes.h"
#include "Functions.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/compiler/CompilerOptions.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/Exceptions.h>

#include <openvdb/openvdb.h>
#include <openvdb_ax/version.h>
#include <openvdb/points/PointDataGrid.h>

#include <unordered_map>

namespace point_functions_internal
{
    void edit_group(const uint8_t* const name,
                    const uint64_t index,
                    void** groupHandles,
                    void* const newDataPtr,
                    const void* const data,
                    const bool flag);
}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {


struct InGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_ingroup", FunctionBase::Point,
            "Internal function for querying point group data")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(in_group)
        }) {}

    private:
       static bool in_group(const uint8_t* const name,
                            const uint64_t index,
                            void** groupHandles,
                            const void* const newDataPtr,
                            const void* const data);
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("ingroup", FunctionBase::Point,
        "Return whether or not the current point is a member of the given group name. "
        "This returns false if the group does not exist.")
    inline static Ptr create(const FunctionOptions&) { return Ptr(new InGroup()); }

    InGroup() : FunctionBase({
        FunctionSignature<bool(StringPtrType)>::create
            (nullptr, std::string("ingroup"))
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_ingroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override {

        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        Internal func;
        return func.execute(internalArgs, globals, builder, M);
    }
};

struct RemoveFromGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_removefromgroup", FunctionBase::Point,
            "Internal function for setting point group data")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(point_functions_internal::edit_group)
        }) {}
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("removefromgroup", FunctionBase::Point,
        "Remove the current point from the given group name, effectively setting its membership "
        "to false. This function has no effect if the group does not exist.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new RemoveFromGroup()); }

    RemoveFromGroup() : FunctionBase({
        FunctionSignature<void(StringPtrType)>::create
            (nullptr, std::string("removefromgroup"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_removefromgroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {

        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        internalArgs.emplace_back(llvm::ConstantInt::get(LLVMType<bool>::get(builder.getContext()), false));

        Internal func;
        return func.execute(internalArgs, globals, builder, M);
    }
};

struct AddToGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_addtogroup", FunctionBase::Point,
            "Internal function for setting point group data")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(point_functions_internal::edit_group)
        }) {}
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("addtogroup", FunctionBase::Point,
        "Add the current point to the given group name, effectively setting its membership "
        "to true. If the group does not exist, it is implicitly created. This function has "
        "no effect if the point already belongs to the given group.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new AddToGroup()); }

    AddToGroup() : FunctionBase({
        FunctionSignature<void(StringPtrType)>::create
            (nullptr, std::string("addtogroup"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_addtogroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {

        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        internalArgs.emplace_back(llvm::ConstantInt::get(LLVMType<bool>::get(builder.getContext()), true));

        Internal func;
        return func.execute(internalArgs, globals, builder, M);
    }
};

struct DeletePoint : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("deletepoint", FunctionBase::Point,
        "Delete the current point from the point set. Note that this does not stop AX execution - "
        "any additional AX commands will be executed on the point and it will remain accessible "
        "until the end of execution.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new DeletePoint()); }

    DeletePoint() : FunctionBase({
        FunctionSignature<void()>::create
            (nullptr, std::string("deletepoint"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_addtogroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {

        llvm::Value* store = llvmStringToValue("dead", builder);

        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(store);

        AddToGroup func;
        return func.execute(internalArgs, globals, builder, M);
    }
};

struct SetAttribute : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("setattribute", FunctionBase::Point,
        "Internal function for setting the value of a point attribute.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new SetAttribute()); }

    SetAttribute() : FunctionBase({
        // pod types pass by value
        DECLARE_FUNCTION_SIGNATURE(set_attribute<double>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute<float>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute<int64_t>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute<int32_t>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute<int16_t>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute<bool>),
        // non-pod types pass by ptr
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::Vec3d>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::Vec3f>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::Vec3i>),
        // DECLARE_FUNCTION_SIGNATURE(set_attribute_string),
    }) {}

private:

    template <typename ValueT>
    inline static void set_attribute_ptr(void* attributeHandle, const uint64_t index, const ValueT* value)
    {
        using AttributeHandleType = openvdb::points::AttributeWriteHandle<ValueT>;

        assert(attributeHandle);
        assert(value);

        AttributeHandleType* handle = static_cast<AttributeHandleType*>(attributeHandle);
        handle->set(index, *value);
    }

    template <typename ValueT>
    inline static void set_attribute(void* attributeHandle, const uint64_t index, const ValueT value)
    {
        set_attribute_ptr<ValueT>(attributeHandle, index, &value);
    }

    static void set_attribute_string(void *attributeHandle,
                                     const uint64_t index,
                                     const uint8_t* value,
                                     void* const newDataPtr);
};

struct SetPointPWS : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("setpointpws", FunctionBase::Point,
        "Internal function for setting the value of a the world space position of the point.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new SetPointPWS()); }

    SetPointPWS() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(set_point_pws)
    }) {}

private:

    static void set_point_pws(void* newDataPtr,
                              const uint64_t index,
                              openvdb::Vec3s* value);
};

//@todo: Once required improvements made to StringAttributeArray
//       renable string attributes and functions
// struct StringAttribSize : public FunctionBase
// {
//     DEFINE_IDENTIFIER_CONTEXT_DOC("strattribsize", FunctionBase::Point,
//         "Internal function for querying the size of a points string attribute")

//     inline static Ptr create(const FunctionOptions&) { return Ptr(new StringAttribSize()); }

//     StringAttribSize() : FunctionBase({
//         DECLARE_FUNCTION_SIGNATURE(str_attrib_size)
//     }) {}

// private:
//     static size_t str_attrib_size(void* attributeHandle,
//                                   const uint64_t index,
//                                   const void* const newDataPtr);

// };


struct GetAttribute : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getattribute", FunctionBase::Point,
        "Internal function for getting the value of a point attribute.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetAttribute()); }

    GetAttribute() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<double>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<float>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int64_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int32_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int16_t>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<bool>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::Vec3d>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::Vec3f>, 1),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::Vec3i>, 1),
        // DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute_string, 1)
    }) {}

private:
    template <typename ValueT>
    inline static void get_attribute(void* attributeHandle, const uint64_t index, ValueT* value)
    {
        // typedef is a read handle. As write handles are derived types this
        // is okay and lets us define the handle types outside IR for attributes that are
        // only being read!

        using AttributeHandleType = openvdb::points::AttributeHandle<ValueT>;

        assert(attributeHandle);
        assert(value);
        AttributeHandleType* handle = static_cast<AttributeHandleType*>(attributeHandle);

        (*value) = handle->get(index);
    }

    static void get_attribute_string(void *attributeHandle,
                                     const uint64_t index,
                                     uint8_t* value,
                                     const void* const newDataPtr);
};

struct GetPointPWS : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("getpointpws", FunctionBase::Point,
        "Internal function for getting the value of a the world space position of the point.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetPointPWS()); }

    GetPointPWS() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_point_pws, 1)
    }) {}

private:

    static void get_point_pws(void* newDataPtr,
                              const uint64_t index,
                              openvdb::Vec3s* value);
};

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_POINT_FUNCTIONS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
