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

/// @file codegen/Types.h
///
/// @authors Nick Avramoussis
///
/// @brief  Consolidated llvm types for most supported types
///

#ifndef OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED

#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/Exceptions.h>

#include <openvdb/math/Mat3.h>
#include <openvdb/math/Mat4.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/Types.h>

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#include <type_traits>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

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
        OPENVDB_THROW(LLVMTypeError, "LLVMType called with an unsupported type \"" +
            std::string(typeNameAsString<T>()) + "\".");
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
} \

REGISTER_LLVM_TYPE_MAP(bool, llvm::Type::getInt1Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int8_t, llvm::Type::getInt8Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int16_t, llvm::Type::getInt16Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int32_t, llvm::Type::getInt32Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(int64_t, llvm::Type::getInt64Ty, llvm::ConstantInt::getSigned);
REGISTER_LLVM_TYPE_MAP(float, llvm::Type::getFloatTy, llvm::ConstantFP::get);
REGISTER_LLVM_TYPE_MAP(double, llvm::Type::getDoubleTy, llvm::ConstantFP::get);

template <>
struct LLVMType<char> {
    static_assert(std::is_same<uint8_t, unsigned char>::value,
        "This library requires std::uint8_t to be implemented as unsigned char.");
    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        return LLVMType<uint8_t>::get(C);
    }
};

template <>
struct LLVMType<std::string> {
    static inline llvm::StructType*
    get(llvm::LLVMContext& C) {
        const std::vector<llvm::Type*> types {
            LLVMType<char*>::get(C),  // array
            LLVMType<int64_t>::get(C) // size
        };
        return llvm::StructType::get(C, types, "string");
    }
    static inline llvm::Value*
    get(llvm::LLVMContext& C, llvm::Constant* string, llvm::Constant* size) {
        return llvm::ConstantStruct::get(LLVMType<std::string>::get(C), {string, size});
    }
    /// @note Creating strings from a literal requires a GEP instruction to
    ///   store the string ptr on the struct.
    /// @note Usually you should be using s = builder.CreateGlobalStringPtr()
    ///   followed by LLVMType<std::string>::get(C, s) rather than allocating
    ///   a non global string
    static inline llvm::Value*
    get(llvm::LLVMContext& C, const std::string& string, llvm::IRBuilder<>& builder) {
        llvm::Constant* constant =
            llvm::ConstantDataArray::getString(C, string, /*terminator*/true);
        llvm::Constant* size = llvm::cast<llvm::Constant>
            (LLVMType<int64_t>::get(C, static_cast<int64_t>(string.size())));
        llvm::Value* zero = LLVMType<int32_t>::get(C, 0);
        llvm::Value* args[] = { zero, zero };
        constant = llvm::cast<llvm::Constant>
            (builder.CreateInBoundsGEP(constant->getType(), constant, args));
        return LLVMType<std::string>::get(C, constant, size);
    }
};

template <>
struct LLVMType<void> {
    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        return llvm::Type::getVoidTy(C);
    }
};

template <>
struct LLVMType<uintptr_t> {
    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        return llvm::Type::getIntNTy(C, static_cast<unsigned>(/*bits*/sizeof(uintptr_t)*CHAR_BIT));
    }
    template <typename T>
    static inline llvm::Constant*
    get(llvm::LLVMContext& C, const T& value) {
        return llvm::ConstantInt::get(LLVMType<uintptr_t>::get(C), reinterpret_cast<uintptr_t>(&value));
    }
};

/// @note  void/opaque pointers are not supported in LLVM (although they seem to
///        work as long as assertions are turned off during LLVM compilation). We
///        cant use i8* types instead as various code-generation checks the llvm
///        type for instruction branching. To temporarily circumvent this, void
///        pointers are re-directed as pointers to structs containing a void member.
///        They should never be used internally, and are only required by the external
///        kernel functions.
///
/// @todo  introduce better front-end type management of AX types
///
template <>
struct LLVMType<void*> {
    static inline llvm::Type*
    get(llvm::LLVMContext& C) {
        llvm::Type* type = LLVMType<void>::get(C);
        return llvm::StructType::get(C,
            llvm::ArrayRef<llvm::Type*>(type))->getPointerTo(0);
    }
};

template <> struct LLVMType<math::Vec2<int32_t>> : public LLVMType<int32_t[2]> {};
template <> struct LLVMType<math::Vec2<float>> : public LLVMType<float[2]> {};
template <> struct LLVMType<math::Vec2<double>> : public LLVMType<double[2]> {};
template <> struct LLVMType<math::Vec3<int32_t>> : public LLVMType<int32_t[3]> {};
template <> struct LLVMType<math::Vec3<float>> : public LLVMType<float[3]> {};
template <> struct LLVMType<math::Vec3<double>> : public LLVMType<double[3]> {};
template <> struct LLVMType<math::Vec4<int32_t>> : public LLVMType<int32_t[3]> {};
template <> struct LLVMType<math::Vec4<float>> : public LLVMType<float[3]> {};
template <> struct LLVMType<math::Vec4<double>> : public LLVMType<double[3]> {};
template <> struct LLVMType<math::Mat3<float>> : public LLVMType<float[9]> {};
template <> struct LLVMType<math::Mat3<double>> : public LLVMType<double[9]> {};
template <> struct LLVMType<math::Mat4<float>> : public LLVMType<float[16]> {};
template <> struct LLVMType<math::Mat4<double>> : public LLVMType<double[16]> {};

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
/// @param type  The name of the type to request.
/// @param C     The LLVMContext to request the Type from.
///
inline llvm::Type*
llvmTypeFromToken(const ast::tokens::CoreType& type,
                  llvm::LLVMContext& C)
{
    switch (type) {
        case ast::tokens::BOOL    : return LLVMType<bool>::get(C);
        case ast::tokens::SHORT   : return LLVMType<int16_t>::get(C);
        case ast::tokens::INT     : return LLVMType<int32_t>::get(C);
        case ast::tokens::LONG    : return LLVMType<int64_t>::get(C);
        case ast::tokens::FLOAT   : return LLVMType<float>::get(C);
        case ast::tokens::DOUBLE  : return LLVMType<double>::get(C);
        case ast::tokens::VEC2I   : return LLVMType<int32_t[2]>::get(C);
        case ast::tokens::VEC2F   : return LLVMType<float[2]>::get(C);
        case ast::tokens::VEC2D   : return LLVMType<double[3]>::get(C);
        case ast::tokens::VEC3I   : return LLVMType<int32_t[3]>::get(C);
        case ast::tokens::VEC3F   : return LLVMType<float[3]>::get(C);
        case ast::tokens::VEC3D   : return LLVMType<double[3]>::get(C);
        case ast::tokens::VEC4I   : return LLVMType<int32_t[4]>::get(C);
        case ast::tokens::VEC4F   : return LLVMType<float[4]>::get(C);
        case ast::tokens::VEC4D   : return LLVMType<double[4]>::get(C);
        case ast::tokens::MAT3F   : return LLVMType<float[9]>::get(C);
        case ast::tokens::MAT3D   : return LLVMType<double[9]>::get(C);
        case ast::tokens::MAT4F   : return LLVMType<float[16]>::get(C);
        case ast::tokens::MAT4D   : return LLVMType<double[16]>::get(C);
        case ast::tokens::STRING  : return LLVMType<std::string>::get(C);
        case ast::tokens::UNKNOWN :
        default      :
            OPENVDB_THROW(LLVMTypeError, "Attribute Type not recognised");
    }
}

inline ast::tokens::CoreType
tokenFromLLVMType(const llvm::Type* type)
{
    if (type->isPointerTy()) {
        type = type->getPointerElementType();
    }
    if (type->isIntegerTy(1))   return ast::tokens::BOOL;
    if (type->isIntegerTy(16))  return ast::tokens::SHORT;
    if (type->isIntegerTy(32))  return ast::tokens::INT;
    if (type->isIntegerTy(64))  return ast::tokens::LONG;
    if (type->isFloatTy())      return ast::tokens::FLOAT;
    if (type->isDoubleTy())     return ast::tokens::DOUBLE;
    if (type->isArrayTy()) {
        llvm::Type* element = type->getArrayElementType();
        const size_t size = type->getArrayNumElements();
        if (size == 2) {
            if (element->isIntegerTy(32))  return ast::tokens::VEC2I;
            if (element->isFloatTy())      return ast::tokens::VEC2F;
            if (element->isDoubleTy())     return ast::tokens::VEC2D;
        }
        else if (size == 3) {
            if (element->isIntegerTy(32))  return ast::tokens::VEC3I;
            if (element->isFloatTy())      return ast::tokens::VEC3F;
            if (element->isDoubleTy())     return ast::tokens::VEC3D;
        }
        else if (size == 4) {
            if (element->isIntegerTy(32))  return ast::tokens::VEC4I;
            if (element->isFloatTy())      return ast::tokens::VEC4F;
            if (element->isDoubleTy())     return ast::tokens::VEC4D;
        }
        else if (size == 9) {
            if (element->isFloatTy())      return ast::tokens::MAT3F;
            if (element->isDoubleTy())     return ast::tokens::MAT3D;
        }
        else if (size == 16) {
            if (element->isFloatTy())      return ast::tokens::MAT4F;
            if (element->isDoubleTy())     return ast::tokens::MAT4D;
        }
    }
    // return string token for both int8 and std::string types
    if (type->isIntegerTy(8))   return ast::tokens::STRING;
    if (type == LLVMType<std::string>::get(type->getContext())) {
        return ast::tokens::STRING;
    }
    return ast::tokens::UNKNOWN;
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

/// @brief Returns whether the supplied Type* is an array type with count elements
/// @param type The Type* to check
/// @param count The number of elements
///
inline bool
isArrayNType(llvm::Type* type, const size_t count)
{
    llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
    if (!arrayType) return false;
    return arrayType->getNumElements() == count;
}

/// @brief Returns whether the supplied Type* is an array type with 3 elements
/// @param type The Type* to check
///
inline bool
isArray3Type(llvm::Type* type)
{
    return isArrayNType(type, 3);
}


namespace internal
{

inline bool isValidGlobalToken(const std::string& token)
{
    static const std::vector<char> sKeys { '@', '$' };
    for (const char key : sKeys) {
        size_t pos = token.find(key);
        if (pos == std::string::npos) continue;
        pos+=1;
        for (const char key : sKeys) {
            if (token.find(key, pos) != std::string::npos) return false;
        }
        return true;
    }
    return false;
}

} // internal

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
    assert(internal::isValidGlobalToken(global));
    type = global.substr(0, at);
    name = global.substr(at + 1, global.size());
    return true;
}

/// @brief  Parse a global variable name to figure out if it is an external variable
///         access. Returns true if it is a valid access and sets name and type to
///         the corresponding values.
///
/// @param  global  The global token name
/// @param  name    The name to set if the token is a valid external variable access
/// @param  type    The type to set if the token is a valid external variable access
///
inline bool
isGlobalExternalAccess(const std::string& global,
                       std::string& name,
                       std::string& type)
{
    const size_t at = global.find("$");
    if (at == std::string::npos) return false;
    assert(internal::isValidGlobalToken(global));
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
    const std::string global = type + "@" + name;
    assert(internal::isValidGlobalToken(global));
    return global;
}

/// @brief  Returns a global token name representing a valid external variable access
///         from a given name and type.
/// @note   The type is not validated but must be one of the supported typenames.
///         See llvmTypeFromName.
///
/// @param  name    The attribute name
/// @param  type    The attribute type
///
inline std::string
getGlobalExternalAccess(const std::string& name, const std::string& type)
{
    const std::string global = type + "$" + name;
    assert(internal::isValidGlobalToken(global));
    return global;
}

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_TYPES_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
