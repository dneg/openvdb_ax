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

/// @file codegen/PointFunctions.h
///
/// @authors Nick Avramoussis, Richard Jones
///
/// @brief  Contains the function objects that define the functions used in
///         point compute function generation, to be inserted into the
///         FunctionRegistry. These define the functions available when operating
///         on points. Also includes the definitions for the point attribute
///         retrieval and setting.
///
///

#include "Functions.h"
#include "FunctionTypes.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb/openvdb.h>
#include <openvdb/points/PointDataGrid.h>

#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/compiler/CompilerOptions.h>
#include <openvdb_ax/compiler/LeafLocalData.h>
#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/version.h>

#include <unordered_map>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

namespace point_functions_internal
{
    void edit_group(const char* const name,
                    const uint64_t index,
                    void** groupHandles,
                    void* const newDataPtr,
                    const void* const data,
                    const bool flag);
}

// Macro for defining the function identifier and context. All functions must
// define these values

#define DEFINE_IDENTIFIER_DOC(Identifier, Documentation) \
    inline const std::string identifier() const override final { \
        return std::string(Identifier); \
    } \
    inline void getDocumentation(std::string& doc) const override final { \
        doc = Documentation; \
    }

struct InGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_ingroup",
            "Internal function for querying point group data")

        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(in_group)
        }) {}

    private:
       static bool in_group(const char* const name,
                            const uint64_t index,
                            void** groupHandles,
                            const void* const newDataPtr,
                            const void* const data);
    };

    DEFINE_IDENTIFIER_DOC("ingroup", "Return whether or not the current point is "
        "a member of the given group name. This returns false if the group does "
        "not exist.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new InGroup()); }

    InGroup() : FunctionBase({
        FunctionSignature<bool(char*)>::create
            (nullptr, std::string("ingroup"))
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_ingroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override
    {
        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        Internal func;
        return func.execute(internalArgs, globals, B);
    }
};

struct RemoveFromGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_removefromgroup",
            "Internal function for setting point group data")

        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(point_functions_internal::edit_group)
        }) {}
    };

    DEFINE_IDENTIFIER_DOC("removefromgroup", "Remove the current point from the "
        "given group name, effectively setting its membership to false. This "
        "function has no effect if the group does not exist.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new RemoveFromGroup()); }

    RemoveFromGroup() : FunctionBase({
        FunctionSignature<void(char*)>::create
            (nullptr, std::string("removefromgroup"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_removefromgroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        internalArgs.emplace_back(llvm::ConstantInt::get(LLVMType<bool>::get(B.getContext()), false));
        Internal func;
        return func.execute(internalArgs, globals, B);
    }
};

struct AddToGroup : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_addtogroup",
            "Internal function for setting point group data")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(point_functions_internal::edit_group)
        }) {}
    };

    DEFINE_IDENTIFIER_DOC("addtogroup", "Add the current point to the given group "
        "name, effectively setting its membership to true. If the group does not "
        "exist, it is implicitly created. This function has no effect if the point "
        "already belongs to the given group.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new AddToGroup()); }

    AddToGroup() : FunctionBase({
        FunctionSignature<void(char*)>::create
            (nullptr, std::string("addtogroup"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_addtogroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> internalArgs(args);

        internalArgs.emplace_back(globals.at("point_index"));
        internalArgs.emplace_back(globals.at("group_handles"));
        internalArgs.emplace_back(globals.at("leaf_data"));
        internalArgs.emplace_back(globals.at("attribute_set"));

        internalArgs.emplace_back(llvm::ConstantInt::get(LLVMType<bool>::get(B.getContext()), true));
        Internal func;
        return func.execute(internalArgs, globals, B);
    }
};

struct DeletePoint : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("deletepoint",
        "Delete the current point from the point set. Note that this does not "
        "stop AX execution - any additional AX commands will be executed on the "
        "point and it will remain accessible until the end of execution.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new DeletePoint()); }

    DeletePoint() : FunctionBase({
        FunctionSignature<void()>::create
            (nullptr, std::string("deletepoint"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("addtogroup");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>&,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        // args guaranteed to be empty
        llvm::Value* loc = B.CreateGlobalStringPtr("dead"); // char*
        AddToGroup func;
        return func.execute({loc}, globals, B);
    }
};

struct SetAttribute : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("setattribute",
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
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec2<double>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec2<float>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec2<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec3<double>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec3<float>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec3<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec4<double>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec4<float>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Vec4<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Mat3<double>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_ptr<openvdb::math::Mat4<double>>),
        DECLARE_FUNCTION_SIGNATURE(set_attribute_string),
    }) {}

private:

    template <typename ValueT>
    inline static void set_attribute_ptr(void* attributeHandle, const uint64_t index, const ValueT* value)
    {
        using AttributeHandleType = openvdb::points::AttributeWriteHandle<ValueT>;

        assert(attributeHandle);
        assert(value);
        assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

        AttributeHandleType* handle = static_cast<AttributeHandleType*>(attributeHandle);
        handle->set(static_cast<openvdb::Index>(index), *value);
    }

    template <typename ValueT>
    inline static void set_attribute(void* attributeHandle, const uint64_t index, const ValueT value)
    {
        set_attribute_ptr<ValueT>(attributeHandle, index, &value);
    }

    static void set_attribute_string(void *attributeHandle,
                                     const uint64_t index,
                                     const char* value,
                                     void* const newDataPtr);
};

struct StringAttribSize : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("strattribsize", "Internal function for querying the "
        "size of a points string attribute")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new StringAttribSize()); }

    StringAttribSize() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(str_attrib_size)
    }) {}

private:
    static size_t str_attrib_size(void* attributeHandle,
                                  const uint64_t index,
                                  const void* const newDataPtr);
};

struct GetAttribute : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("getattribute", "Internal function for getting the "
        "value of a point attribute.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new GetAttribute()); }

    GetAttribute() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<double>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<float>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int64_t>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int32_t>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<int16_t>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<bool>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec2<double>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec2<float>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec2<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec3<double>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec3<float>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec3<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec4<double>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec4<float>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Vec4<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Mat3<double>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute<openvdb::math::Mat4<double>>),
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(get_attribute_string)
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
        assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

        AttributeHandleType* handle = static_cast<AttributeHandleType*>(attributeHandle);
        (*value) = handle->get(static_cast<openvdb::Index>(index));
    }

    static void get_attribute_string(void *attributeHandle,
                                     const uint64_t index,
                                     char* value,
                                     const void* const newDataPtr);
};

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

void edit_group(const char* const name,
                const uint64_t index,
                void** groupHandles,
                void* const leafDataPtr,
                const void* const data,
                const bool flag)
{
    assert(name);
    if (name[0] == '\0') return;

    const std::string nameStr(name);

    // Get the group handle out of the pre-existing container of handles if they
    // exist

    openvdb::points::GroupWriteHandle* handle = nullptr;
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
    handle->set(static_cast<openvdb::Index>(index), flag);
}

}

bool InGroup::Internal::in_group(const char* const name,
                                 const uint64_t index,
                                 void** groupHandles,
                                 const void* const leafDataPtr,
                                 const void* const data)
{
    assert(name);
    assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

    if (name[0] == '\0') return false;
    if (!groupHandles) return false;

    const std::string nameStr(name);
    const openvdb::points::GroupHandle* handle =
        point_functions_internal::groupHandle<openvdb::points::GroupHandle>(nameStr, groupHandles, data);
    if (handle) return handle->get(static_cast<openvdb::Index>(index));

    // If the handle doesn't exist, check to see if any new groups have
    // been added

    const openvdb::ax::compiler::LeafLocalData* const leafData =
        static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);
    handle = leafData->get(nameStr);
    return handle ? handle->get(static_cast<openvdb::Index>(index)) : false;
}

size_t StringAttribSize::str_attrib_size(void* attributeHandle,
                                         const uint64_t index,
                                         const void* const leafDataPtr)
{
    using AttributeHandleType = openvdb::points::StringAttributeHandle;

    assert(attributeHandle);
    assert(leafDataPtr);
    assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

    const AttributeHandleType* const handle =
        static_cast<AttributeHandleType*>(attributeHandle);

    const openvdb::ax::compiler::LeafLocalData* const leafData =
        static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

    std::string data;
    if (!leafData->getNewStringData(&(handle->array()), index, data)) {
        handle->get(data, static_cast<openvdb::Index>(index));
    }

    // include size of null terminator
    return data.size() + 1;
}

void SetAttribute::set_attribute_string(void* attributeHandle,
                                        const uint64_t index,
                                        const char* value,
                                        void* const leafDataPtr)
{
    using AttributeHandleType = openvdb::points::StringAttributeWriteHandle;

    assert(attributeHandle);
    assert(value);
    assert(leafDataPtr);
    assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

    const std::string s(value);

    AttributeHandleType* const handle =
        static_cast<AttributeHandleType*>(attributeHandle);

    openvdb::ax::compiler::LeafLocalData* const leafData =
        static_cast<openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

    // Check to see if the string exists in the metadata cache. If so, set the string and
    // remove any new data associated with it, otherwise set the new data

    if (handle->contains(s)) {
        handle->set(static_cast<openvdb::Index>(index), s);
        leafData->removeNewStringData(&(handle->array()), index);
    }
    else {
        leafData->setNewStringData(&(handle->array()), index, s);
    }
}

void GetAttribute::get_attribute_string(void* attributeHandle,
                                        const uint64_t index,
                                        char* value,
                                        const void* const leafDataPtr)
{
    using AttributeHandleType = openvdb::points::StringAttributeHandle;

    assert(attributeHandle);
    assert(value);
    assert(leafDataPtr);
    assert(index < static_cast<uint64_t>(std::numeric_limits<openvdb::Index>::max()));

    AttributeHandleType* const handle =
        static_cast<AttributeHandleType*>(attributeHandle);
    const openvdb::ax::compiler::LeafLocalData* const leafData =
        static_cast<const openvdb::ax::compiler::LeafLocalData* const>(leafDataPtr);

    std::string data;
    if (!leafData->getNewStringData(&(handle->array()), index, data)) {
        handle->get(data, static_cast<openvdb::Index>(index));
    }

    strcpy(value, data.c_str());
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


void insertVDBPointFunctions(FunctionRegistry& registry,
    const FunctionOptions* options)
{
    const bool create = options && !options->mLazyFunctions;
    auto add = [&](const std::string& name,
        const FunctionRegistry::ConstructorT creator,
        const bool internal)
    {
        if (create) registry.insertAndCreate(name, creator, *options, internal);
        else        registry.insert(name, creator, internal);
    };

    // point functions

    add("addtogroup", AddToGroup::create, false);
    add("ingroup", InGroup::create, false);
    add("removefromgroup", RemoveFromGroup::create, false);
    add("deletepoint", DeletePoint::create, false);

    // internal point functions

    add("getattribute", GetAttribute::create, true);
    add("setattribute", SetAttribute::create, true);
    add("strattribsize", StringAttribSize::create, true);

    // indirect internals

    add("internal_addtogroup", AddToGroup::Internal::create, true);
    add("internal_ingroup", InGroup::Internal::create, true);
    add("internal_removefromgroup", RemoveFromGroup::Internal::create, true);
}

} // namespace codegen
} // namespace ax
} // namespace openvdb_version
} // namespace openvdb


// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
