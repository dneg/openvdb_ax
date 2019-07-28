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
///////////////////////////////////////////////////////////////////////////

/// @file codegen/FunctionTypes.h
///
/// @authors Nick Avramoussis
///
/// @brief  Definitions for function types and function signatures for
///         automatic conversion, registration and execution within the
///         compute generation
///

#ifndef OPENVDB_AX_CODEGEN_FUNCTION_TYPES_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_FUNCTION_TYPES_HAS_BEEN_INCLUDED

#include <openvdb_ax/codegen/Types.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include <functional>
#include <memory>
#include <stack>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

class PrioritiseIRGeneration {};

/// @brief  Parse a set amount of llvm Values from the stack and "format" them
///         such that they are in the default expected state. This state defined
///         scalars to be loaded, arrays to be pointers to the underlying array and
///         strings to be a pointer to the start of the string.
///
/// @param arguments  A vector of llvm arguments to populate
/// @param values     The value stack from the computer generation
/// @param count      The number of values to pop from the stack
/// @param builder    The current llvm IRBuilder
///
void
stackValuesForFunction(std::vector<llvm::Value*>& arguments,
                   std::stack<llvm::Value*>& values,
                   const size_t count,
                   llvm::IRBuilder<>& builder);


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Object to array conversion methods to allow functions to return vector types.
///         These containers provided an interface for automatic conversion of C++ objects
///         to LLVM types as array types.

template <typename T, size_t _SIZE = 1>
struct ArgType {
    using Type = T;
    static const size_t SIZE = _SIZE;
    using ArrayType = Type[SIZE];
};

template <typename T, size_t S>
struct LLVMType<ArgType<T,S>> : public LLVMType<T[S]> {};

using V2D = ArgType<double, 2>;
using V2F = ArgType<float, 2>;
using V2I = ArgType<int32_t, 2>;
using V3D = ArgType<double, 3>;
using V3F = ArgType<float, 3>;
using V3I = ArgType<int32_t, 3>;
using V4D = ArgType<double, 4>;
using V4F = ArgType<float, 4>;
using V4I = ArgType<int32_t, 4>;
using M3D = ArgType<double, 9>;
using M3F = ArgType<float, 9>;
using M4D = ArgType<double, 16>;
using M4F = ArgType<float, 16>;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Templated function traits which take a std::function object and provides
///          compile-time index access to the types of the function signature
///
template<typename T>
struct FunctionTraits;

template<typename ReturnT, typename ...Args>
struct FunctionTraits<std::function<ReturnT(Args...)>>
{
    using ReturnType = ReturnT;
    static const size_t N_ARGS = sizeof...(Args);
    template <size_t I>
    struct Arg {
        using Type = typename std::tuple_element<I, std::tuple<Args...>>::type;
        static_assert(!std::is_reference<Type>::value,
            "Reference types/arguments are not supported for automatic "
            "LLVM Type conversion. Use pointers instead.");
    };
};

/// @brief  Templated argument iterator which takes a std::function object and
///          an argument index and provides compile time instantiation of function
///          argument type conversions to corresponding llvm values
///
template <typename FunctionT, size_t I>
struct ArgumentIterator
{
    using ArgumentValueType =
        typename FunctionTraits<FunctionT>::template Arg<I-1>::Type;

    static void iter(std::vector<llvm::Type*>& args, llvm::LLVMContext& C) {
        ArgumentIterator<FunctionT, I-1>::iter(args, C);
        args.emplace_back(LLVMType<ArgumentValueType>::get(C));
    }
};

template <typename FunctionT>
struct ArgumentIterator<FunctionT, 0> {
    static void iter(std::vector<llvm::Type*>&, llvm::LLVMContext&) {}
};

/// @brief  Populate a vector of llvm types from a function signature object.
///         The signature should be stored in an std::function object. Returns
///         the return type of the function signature.
/// @note   Reference arguments are not supported.
///
/// @param C  The llvm context
/// @param types   A vector of types to populate
///
template<typename FunctionT>
inline llvm::Type*
llvmTypesFromFunction(llvm::LLVMContext& C, std::vector<llvm::Type*>* types)
{
    using Traits = FunctionTraits<FunctionT>;
    using ArgumentIteratorT =
        ArgumentIterator<FunctionT, Traits::N_ARGS>;

    if (types) {
        types->reserve(Traits::N_ARGS);
        ArgumentIteratorT::iter(*types, C);
    }

    return LLVMType<typename Traits::ReturnType>::get(C);
}

/// @brief  Populate a vector of llvm types from a function signature declaration.
///
/// @param C  The llvm context
/// @param types   A vector of types to populate
///
template <typename Signature>
inline llvm::Type*
llvmTypesFromSignature(llvm::LLVMContext& C, std::vector<llvm::Type*>* types)
{
    using FunctionT = std::function<Signature>;
    return llvmTypesFromFunction<FunctionT>(C, types);
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  The base definition for a single instance of a function signature. A
///         registered function type built with an instance of a FunctionBase can
///         store multiple function signatures.
///
struct FunctionSignatureBase
{
    using Ptr = std::shared_ptr<FunctionSignatureBase>;
    using VoidFPtr = void(*)();

    /// @brief  An enum used to describe how closely a container of llvm types
    ///         matches the stored function signature. This is used to automatically
    ///         find the closest matching function signature from a set of
    ///         llvm value argument
    ///
    enum class SignatureMatch
    {
        None = 0,
        Size,
        Implicit,
        Explicit
    };

    /// @brief  Pure virtual function which returns the size of the function signature.
    virtual size_t size() const = 0;

    /// @brief  Pure virtual function which returns the function return type as an llvm type
    ///         and optionally populates a vector of llvm types representing the definition
    ///         of the function arguments
    ///
    /// @param  C      The llvm context
    /// @param  types  An optional vector of llvm types to populate
    ///
    virtual llvm::Type*
    toLLVMTypes(llvm::LLVMContext& C, std::vector<llvm::Type*>* types = nullptr) const = 0;

    /// @brief  Returns the pointer to the globally accessible function. Note that
    ///         This can be null if the function is entirely IR based
    ///
    inline VoidFPtr functionPointer() const { return mFunction; }

    /// @brief  Returns the function symbol name. For functions requiring external
    ///         linkage, this is the name of the function declaration.
    ///
    inline const std::string& symbolName() const { return mSymbolName; }

    /// @brief  Returns the best possible signature match from a vector of LLVM types
    ///         representing the function arguments.
    ///
    /// @param  types  A vector of llvm types representing the function argument types
    ///
    SignatureMatch match(const std::vector<llvm::Type*>& types) const;

    /// @brief  Returns true if the provided size matches the required number of
    ///         arguments for this function signature.
    ///
    /// @param  size  The size of the function signature to check
    ///
    bool sizeMatch(const size_t size) const;

    /// @brief  Returns true if the provided vector of llvm types matches this function
    ///         signature exactly with no additional casting required.
    ///
    /// @param  input  A vector of llvm types representing the function argument types
    ///
    bool explicitMatch(const std::vector<llvm::Type*>& input) const;

    /// @brief  Returns true if the provided vector of llvm types matches this function
    ///         signature with additional but supported implicit casting
    ///
    /// @param  input  A vector of llvm types representing the function argument types
    ///
    bool implicitMatch(const std::vector<llvm::Type*>& input) const;

    /// @brief  Returns true if this function has a return value which is not void.
    ///         Does not include functions with return values as output arguments.
    ///
    /// @param  C  The llvm context
    ///
    bool hasReturnValue(llvm::LLVMContext& C) const;

    /// @brief  Returns true if this function has arguments which return a value
    ///
    inline bool hasOutputArgument() const { return mLastArgumentReturn; }

    /// @brief  Allocate and return the output argument of this function. This is useful
    ///         when constructing the correct llvm values to pass in as arguments due to
    ///         the front facing signatures requiring different output locations.
    /// @note   Does nothing and returns a nullptr if this function does not have an
    ///         output argument
    ///
    /// @param builder  The current llvm IRBuilder
    ///
    llvm::Value* getOutputArgument(llvm::IRBuilder<>& B) const;

    /// @brief  Appends the output argument types to a vector of llvm types.
    /// @note   This does not include the function return type
    ///
    /// @param C      The llvm context
    ///
    llvm::Type* getOutputType(llvm::LLVMContext& C) const;

    /// @brief  Builds and registers this function signature and symbol name as an
    ///         available and callable function within the llvm module. Returns the
    ///         resulting llvm function
    ///
    /// @param M  The llvm module to insert the function into
    ///
    llvm::Function* toLLVMFunction(llvm::Module& M) const;

    /// @brief  Prints a description of this functions return type, name and signature
    ///
    /// @param C        The llvm context
    /// @param name     The name of the function to insert after it's return type
    /// @param os       The string stream to write to
    /// @param axtypes  Whether to print the types as AX string types or LLVM types.
    ///                 If AX types, various assumptions are made about the forward
    ///                 facing type. Pointers are ignored and strings are used for
    ///                 char*/uint8* access
    ///
    void print(llvm::LLVMContext& C,
        const std::string& name = "",
        std::ostream& os = std::cout,
        const bool axtypes = true) const;

protected:

    /// @brief  Base Function Signature constructor, expected to be instantiated by
    ///         derived classes.
    ///
    /// @param function  A void pointer to the available function if it is an external
    ///                  function
    /// @param symbol    The function symbol name. This must be the name of the function
    ///                  signature declaration if the function is external
    /// @param lastArgIsReturn   Whether the last argument of the function signature is
    ///                  the return value
    ///
    FunctionSignatureBase(VoidFPtr function,
        const std::string& symbol,
        const bool lastArgIsReturn = false);
    virtual ~FunctionSignatureBase() = default;

    /// @brief  static method for converting a function signatures to a vector of
    ///         llvm types. See llvmTypesFromFunction.
    ///
    /// @param C      The llvm context
    /// @param types  An optional vector of llvm types
    ///
    template<typename FunctionT>
    static llvm::Type*
    toLLVMTypes(llvm::LLVMContext& C, std::vector<llvm::Type*>* types)
    {
        return llvmTypesFromFunction<FunctionT>(C, types);
    }

protected:
    VoidFPtr mFunction;
    const std::string mSymbolName;
    const bool mLastArgumentReturn;
};

/// @brief  The base definition for a single function type.
///
struct FunctionBase
{
    using Ptr = std::shared_ptr<FunctionBase>;
    using FunctionList = std::vector<FunctionSignatureBase::Ptr>;
    using FunctionMatch =
        std::pair<FunctionSignatureBase::Ptr, FunctionSignatureBase::SignatureMatch>;

    /// @brief  Pure virtual function which returns the function identifier for
    ///         this function type.
    ///
    virtual const std::string identifier() const = 0;

    /// @brief  Populate a vector of strings with the identifiers of any functions which
    ///         this function depends on. This is used by the compiler to ensure global
    ///         mappings are added for requires methods
    ///
    /// @param  identifiers  A vector of function identifiers to populate
    ///
    virtual void getDependencies(std::vector<std::string>& identifiers) const;

    /// @brief  Populate a string with any documentation for this function
    ///
    /// @param  doc A string to populate
    ///
    virtual void getDocumentation(std::string& doc) const;

    /// @brief  Given a vector of llvm types, automatically returns the best possible
    ///         function signature pointer and match type.
    /// @note   The vector of provided llvm types does not need to contain the possible
    ///         output arguments if addOutputArguments is false.
    ///
    /// @param types  A vector of llvm types representing the function argument types
    /// @param C      The llvm context
    /// @param addOutputArguments  Whether to append any output arguments before trying to
    ///        match the provided llvm types
    ///
    FunctionMatch
    match(const std::vector<llvm::Type*>& types,
          llvm::LLVMContext& C,
          const bool addOutputArguments = false) const;

    /// @brief  Given a vector of llvm values provided by the user, find the best possible
    ///         function signature, generate and execute the function body. Returns the
    ///         return value of the function (can be void) and optionally populates a vector
    ///         of return types.
    ///
    /// @param functionArgs A vector of llvm types representing the function argument values
    /// @param globals      The map of global names to llvm::Values
    /// @param builder      The current llvm IRBuilder
    /// @param results      If provided, is populated with any function output arguments if any
    ///                     exist
    /// @param addOutputArguments  Whether the provided llvm value argument vector should
    ///        also append any additional function output values that exist in the matched
    ///        signature
    ///
    virtual llvm::Value*
    execute(const std::vector<llvm::Value*>& functionArgs,
            const std::unordered_map<std::string, llvm::Value*>& globals,
            llvm::IRBuilder<>& builder,
            std::vector<llvm::Value*>* results = nullptr,
            const bool addOutputArguments = true) const;

    /// @brief  Generates the function body. Returns the function return value or a null
    ///         pointer if there is no function body (external function)
    ///
    /// @param args     A vector of llvm types representing the function argument values
    /// @param globals  The map of global names to llvm::Values
    /// @param builder  The current llvm IRBuilder
    ///
    virtual llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
             const std::unordered_map<std::string, llvm::Value*>& globals,
             llvm::IRBuilder<>& builder) const;

    /// @brief  Accessor to the underlying function signature list
    ///
    inline const FunctionList& list() const { return mFunctionList; }

protected:

    /// @brief  Function Base constructor, expected to be instantiated by derived classes
    ///
    /// @param  list A vector of function signatures corresponding to this function type
    ///
    FunctionBase(const FunctionList& list)
        : mFunctionList(list) {}
    virtual ~FunctionBase() = default;

    const FunctionList mFunctionList;
};


/// @brief  A single instance of a function with a unique signature and token
///
/// @todo Improve struct and template consistency and typedefs of std::function<>
///
/// @note This struct is templated on the signature to allow for constant runtime
///       evaluation of the arguments to llvm types. Ideally they would be stored
///       on the object on creation but as the registry is static, if an function
///       persists until program termination an error will occur on destruction of
///       FunctionSignatures.
///
/// @note Due to the way LLVM differentiates between the signs of values (i.e. holds
///       this information in the values rather than the types) the function framework
///       treats unsigned and signed integers as the same types if their widths are
///       identical.
///
template<typename SignatureT>
struct FunctionSignature : public FunctionSignatureBase
{
    using FunctionSignatureT = FunctionSignature<SignatureT>;
    using Signature = std::function<SignatureT>;
    using Traits = FunctionTraits<Signature>;
    using Ptr = std::shared_ptr<FunctionSignatureT>;

    using FunctionPtrT = typename std::add_pointer<SignatureT>::type;

    FunctionSignature(FunctionSignatureBase::VoidFPtr ptr,
        const std::string& symbol,
        const bool lastArgIsReturn = false)
        : FunctionSignatureBase(ptr, symbol, lastArgIsReturn)
        {
            if (lastArgIsReturn) {
                // check there actually are arguments
                if (this->size() == 0) {
                    OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                        "\" has been setup with the last argument as the return value, however the "
                        "provided signature is empty.");
                }
                // check no return value exists
                if (!std::is_same<typename Traits::ReturnType, void>::value) {
                    OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                        "\" has been setup with the last argument as the return value and a "
                        "non void return type.");
                }

                // @note  This resolves compilation issues where N_ARGS is empty, causing out
                //        of range tuple queries or queries into 0 size arguments.
                // @todo  Improve this

                using __SignatureT =
                    typename std::conditional<Traits::N_ARGS == 0,
                        std::function<void(void*)>,
                        Signature>::type;
                static constexpr size_t Index = Traits::N_ARGS == 0 ? 1 : Traits::N_ARGS;
                using ArgumentIteratorT = ArgumentIterator<__SignatureT, Index>;
                using LT = typename ArgumentIteratorT::ArgumentValueType;

                // check output is a pointer (mem will be allocated)

                if (!std::is_pointer<LT>::value) {
                    OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                        "\" has been setup with the last argument as the return value but it is not"
                        "a pointer type.");
                }
            }
        }

    /// @brief  static creation method. Note that this creation method takes a std::function
    ///         object containing the external function pointer expected to match the signature.
    ///         This enforces that only compatible signatures can be provided.
    ///
    /// @note   Throws if the function pointer could not be inferred from the provided std::function.
    ///         This is usually due to providing an object to the first argument. For these cases,
    ///         ensure they are converted to function pointers prior to calling create. For
    ///         example:
    ///
    ///         static auto lambda = [](double a)->double { return a * 5.0; };
    ///         double(*ptr)(double) = lambda;
    ///         FunctionSignature<double(double)>::create(ptr, "mult");
    ///
    /// @param  function The std::function object representing this functions signature.
    /// @param  symbol   The function symbol name if it is external. If this is a C style function
    ///                  (a free function declared with extern "C"), using the name of the function
    ///                  will always ensure linkage is found, regardless of the llvm linkage type.
    /// @param lastArgIsReturn   Whether the last argument of the function signature is the return value
    ///
    static Ptr create(Signature function, const std::string& symbol, const bool lastArgIsReturn = false)
    {
        FunctionSignatureBase::VoidFPtr functionVoidPtr = nullptr;

        if (function) {
            FunctionPtrT* ptr = function.template target<FunctionPtrT>();
            if (!ptr) {
                OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                    "\" does not match expected signature during creation.");
            }
            functionVoidPtr = reinterpret_cast<FunctionSignatureBase::VoidFPtr>(*ptr);
        }

        return Ptr(new FunctionSignatureT(functionVoidPtr, symbol, lastArgIsReturn));
    }

    /// @brief  Automatically returns the number of arguments of this function using
    ///         function traits.
    ///
    inline size_t size() const override final { return Traits::N_ARGS; }

    /// @brief  method for converting a function signatures to a vector of
    ///         llvm types. See llvmTypesFromFunction.
    ///
    /// @param C      The llvm context
    /// @param types  An optional vector of llvm types
    ///
    inline llvm::Type*
    toLLVMTypes(llvm::LLVMContext& C, std::vector<llvm::Type*>* types = nullptr) const override final
    {
        return FunctionSignatureBase::toLLVMTypes<Signature>(C, types);
    }
};

}
}
}
}

/// @brief  Wrapper for safe external function signature registration
///
#define DECLARE_FUNCTION_SIGNATURE(FunctionPtr) \
    openvdb::ax::codegen::FunctionSignature<decltype(FunctionPtr)>\
        ::create(&FunctionPtr, std::string(#FunctionPtr), false)

#define DECLARE_FUNCTION_SIGNATURE_OUTPUT(FunctionPtr) \
    openvdb::ax::codegen::FunctionSignature<decltype(FunctionPtr)>\
        ::create(&FunctionPtr, std::string(#FunctionPtr), true)

#endif // OPENVDB_AX_CODEGEN_FUNCTION_TYPES_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
