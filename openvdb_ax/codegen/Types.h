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

/// @file codegen/Types.h
///
/// @authors Nick Avramoussis
///
/// @brief  Consolidated llvm types for most supported types
///

#ifndef OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED

#include <openvdb_ax/Exceptions.h>

#include <openvdb/math/Mat4.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/Types.h>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <type_traits>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

/// @brief  Parse a global variable name to figure out if it is an attribute access
///         index. Returns true if it is a valid access and sets name and type to
///         the corresponding values.
///
/// @param  global  The global token name
/// @param  name    The name to set if the token is a valid attribute access
/// @param  type    The type to set if the token is a valid attribute access
///
inline bool
isGlobalAttributeAccess(const std::string& global,
                        std::string& name,
                        std::string& type)
{
    const size_t at = global.find("@");
    if (at == std::string::npos) return false;
    type = global.substr(0, at);
    name = global.substr(at + 1, global.size());
    return true;
}

/// @brief  Returns a global token name representing a valid attribute access from
///         a given attribute name and type.
/// @note   The type is not validated but must be one of the supported typenames.
///         See llvmTypeFromName.
///
/// @param  name    The attribute name
/// @param  type    The attribute type
///
inline std::string
getGlobalAttributeAccess(const std::string& name, const std::string& type)
{
    return type + "@" + name;
}

/// Recursive llvm type mapping from pod types
/// @note  llvm::Types do not store information about the value sign, only meta
///        information about the primitive type (i.e. float, int, pointer) and
///        the precision width. LLVMType<uint64_t> will provide the same type as
///        LLVMType<int64_t>
template <typename T>
struct LLVMType {
    static_assert(!std::is_reference<T>::value,
        "Reference types/arguments are not supported for automatic "
        "LLVM Type conversion. Use pointers instead.");

    // @note  As of LLVM 7, getScalarTy<T> can be used from Type.h
    //        which is implemented as below
    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        int bits = sizeof(T) * CHAR_BIT;
        if (std::is_integral<T>::value) {
            return llvm::Type::getIntNTy(C, bits);
        }
        else if (std::is_floating_point<T>::value) {
            switch (bits) {
                case 32: return llvm::Type::getFloatTy(C);
                case 64: return llvm::Type::getDoubleTy(C);
            }
        }
        OPENVDB_THROW(LLVMTypeError, "LLVMType called with an unsupported type");
    }
};

template <typename T, size_t S>
struct LLVMType<T[S]> {
    static_assert(S != 0,
        "Zero size array types are not supported for automatic LLVM "
        "Type conversion");

    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        return llvm::ArrayType::get(LLVMType<T>::get(C), S);
    }
    static inline llvm::Constant*
    get(llvm::LLVMContext& C, const T(&array)[S]) {
        return llvm::ConstantDataArray::get(C, array);
    }
};

template <typename T>
struct LLVMType<T*> {
    static inline llvm::PointerType*
    get(llvm::LLVMContext& C) {
        return LLVMType<T>::get(C)->getPointerTo(0);
    }
};

template <typename T> struct LLVMType<const T> : public LLVMType<T> {};
template <typename T> struct LLVMType<const T*> : public LLVMType<T*> {};

/// Common C++ types requested as llvm types

#define REGISTER_LLVM_TYPE_MAP(ValueType, LLVMTypeCall, LLVMValueCall) \
template <> struct LLVMType<ValueType> { \
    static inline llvm::Type* \
    get(llvm::LLVMContext& C) { \
        return LLVMTypeCall(C); \
    } \
    static inline llvm::Constant* \
    get(llvm::LLVMContext& C, const ValueType value) { \
        using InputType = \
            typename std::conditional<std::is_floating_point<ValueType>::value, double, int64_t>::type; \
        return LLVMValueCall(LLVMType<ValueType>::get(C), InputType(value)); \
    } \
}; \

REGISTER_LLVM_TYPE_MAP(bool, llvm::Type::getInt1Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int8_t, llvm::Type::getInt8Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int16_t, llvm::Type::getInt16Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int32_t, llvm::Type::getInt32Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int64_t, llvm::Type::getInt64Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(uint64_t, llvm::Type::getInt64Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(float, llvm::Type::getFloatTy, llvm::ConstantFP::get);
REGISTER_LLVM_TYPE_MAP(double, llvm::Type::getDoubleTy, llvm::ConstantFP::get);

template <>
struct LLVMType<char> {
    static_assert(std::is_same<uint8_t, unsigned char>::value,
        "This library requires std::uint8_t to be implemented as unsigned char.");
    static inline llvm::Type*
    get(llvm::LLVMContext& c) {
        return LLVMType<uint8_t>::get(c);
    }
};

template <>
struct LLVMType<void> {
    static inline llvm::Type*
    get(llvm::LLVMContext& c) {
        return llvm::Type::getVoidTy(c);
    }
};

template <> struct LLVMType<math::Vec3<int32_t>> : public LLVMType<int32_t[3]> {};
template <> struct LLVMType<math::Vec3<float>> : public LLVMType<float[3]> {};
template <> struct LLVMType<math::Vec3<double>> : public LLVMType<double[3]> {};

#undef REGISTER_LLVM_TYPE_MAP
#undef REGISTER_OPENVDB_VECTOR_LLVM_TYPE_MAP

/// @brief Returns an llvm integer Type given a requested size and context
/// @param size    The size of the integer to request, i.e. bool - 1, int32 - 32 etc.
/// @param context The LLVMContext to request the Type from.
///
inline llvm::Type*
llvmIntType(const size_t size, llvm::LLVMContext& context)
{
    switch (size) {
        case 1 :  return LLVMType<bool>::get(context);
        case 8 :  return LLVMType<int8_t>::get(context);
        case 16 : return LLVMType<int16_t>::get(context);
        case 32 : return LLVMType<int32_t>::get(context);
        case 64 : return LLVMType<int64_t>::get(context);
        default : OPENVDB_THROW(LLVMTypeError, "Invalid integer size");
    }
}


/// @brief Returns an llvm floating point Type given a requested size and context
/// @param size    The size of the float to request, i.e. float - 32, double - 64 etc.
/// @param context The LLVMContext to request the Type from.
///
inline llvm::Type*
llvmFloatType(const size_t size, llvm::LLVMContext& context)
{
    switch (size) {
        case 16 : return LLVMType<half>::get(context);
        case 32 : return LLVMType<float>::get(context);
        case 64 : return LLVMType<double>::get(context);
        default : OPENVDB_THROW(LLVMTypeError, "Invalid float size");
    }
}

/// @brief  Returns an llvm type representing a type defined by a string.
/// @note   For string types, this function returns the element type, not the
///         object type! The llvm type representing a char block of memory
///         is LLVMType<char*>::get(C);
/// @param type    The name of the type to request.
/// @param module  The LLVMContext to request the Type from.
///
inline llvm::Type*
llvmTypeFromName(const std::string& type,
                 llvm::LLVMContext& C)
{
    if (type == openvdb::typeNameAsString<bool>())     return LLVMType<bool>::get(C);
    if (type == openvdb::typeNameAsString<int16_t>())  return LLVMType<int16_t>::get(C);
    if (type == openvdb::typeNameAsString<int32_t>())  return LLVMType<int32_t>::get(C);
    if (type == openvdb::typeNameAsString<int64_t>())  return LLVMType<int64_t>::get(C);
    if (type == openvdb::typeNameAsString<float>())    return LLVMType<float>::get(C);
    if (type == openvdb::typeNameAsString<double>())   return LLVMType<double>::get(C);
    if (type == openvdb::typeNameAsString<math::Vec3<int32_t>>())  return LLVMType<math::Vec3<int32_t>>::get(C);
    if (type == openvdb::typeNameAsString<math::Vec3<float>>())    return LLVMType<math::Vec3<float>>::get(C);
    if (type == openvdb::typeNameAsString<math::Vec3<double>>())   return LLVMType<math::Vec3<double>>::get(C);
    if (type == openvdb::typeNameAsString<std::string>())          return LLVMType<char>::get(C);

    OPENVDB_THROW(LLVMTypeError, "Attribute Type " + type + " not recognised");
}

/// @brief A LLVM TypeID reference to compare against
///
inline unsigned llvmScalarTypeId()
{
    // Handy TypeID reference

    /// Type::HalfTyID      1
    /// Type::FloatTyID     2
    /// Type::DoubleTyID    3
    /// Type::IntegerTyID   10
    /// Type::StructTyID    12
    /// Type::ArrayTyID     13
    /// Type::PointerTyID   14

    return (1 << llvm::Type::HalfTyID   |
            1 << llvm::Type::FloatTyID  |
            1 << llvm::Type::DoubleTyID |
            1 << llvm::Type::IntegerTyID);
}

/// @brief Returns whether the supplied TypeID is a scalar type
/// @param typeId The llvm::Type::TypeID to check
///
inline bool isScalarType(const llvm::Type::TypeID typeId)
{
    return ((1 << typeId) & llvmScalarTypeId());
}

/// @brief Returns whether the supplied Type* is a scalar type
/// @param type The Type* to check
///
inline bool
isScalarType(llvm::Type* type)
{
    return type->isIntegerTy() || type->isFloatingPointTy();
}

/// @brief Returns whether the supplied Type* is a scalar type
/// @param type The Type* to check
/// @param C The LLVM context
///
inline bool
isCharType(llvm::Type* type,
           llvm::LLVMContext& C)
{
    return type == LLVMType<char>::get(C);
}

/// @brief Returns whether the supplied Type* is an array type
/// @param type The Type* to check
///
inline bool
isArrayType(llvm::Type* type)
{
    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
    return static_cast<bool>(arrayType);
}

/// @brief Returns whether the supplied Type* is an array type with 3 elements
/// @param type The Type* to check
///
inline bool
isArray3Type(llvm::Type* type)
{
    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
    if (!arrayType) return false;
    return arrayType->getNumElements() == 3;
}

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
