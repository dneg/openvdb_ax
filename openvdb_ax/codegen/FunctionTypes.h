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

/// @brief  Helper function to retrieve a set amount of arguments from the
///         argument stack
///
/// @param values     The value stack from the computer generation
/// @param count      The number of values to pop from the stack
/// @param arguments  A vector of llvm arguments to populate
///
void
argumentsFromStack(std::stack<llvm::Value*>& values,
                   const size_t count,
                   std::vector<llvm::Value*>& arguments);

/// @brief  Parse a vector of llvm function arguments such that they are
///         in the default expected state. This state defined scalars to be
///         loaded, arrays to be pointers to the underlying array and strings
///         to be a pointer to the start of the string.
///
/// @param arguments            The vector of llvm arguments to parse
/// @param builder              The current llvm IRBuilder
/// @param loadScalarArguments  Whether to load scalar arguments
///
bool
parseDefaultArgumentState(std::vector<llvm::Value*>& arguments,
                          llvm::IRBuilder<>& builder,
                          const bool loadScalarArguments = true);

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

using StringPtrType = const uint8_t* const;
using V3D = ArgType<double, 3>;
using V3F = ArgType<float, 3>;
using V3I = ArgType<int, 3>;

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
/// @param LLVMContext  The llvm context
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
/// @param LLVMContext  The llvm context
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
    inline void* functionPointer() const { return mFunction; }

    /// @brief  Returns the function symbol name. For functions requiring external
    ///         linkage, this is the name of the function declaration.
    ///
    inline const std::string& symbolName() const { return mSymbolName; }

    /// @brief  Returns the best possible signature match from a vector of LLVM types
    ///         representing the function arguments.
    ///
    /// @param  types  A vector of llvm types representing the function argument types
    /// @param  C      The llvm context
    ///
    SignatureMatch match(const std::vector<llvm::Type*>& types, llvm::LLVMContext& C) const;

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
    /// @param  C      The llvm context
    ///
    bool explicitMatch(const std::vector<llvm::Type*>& input, llvm::LLVMContext& C) const;

    /// @brief  Returns true if the provided vector of llvm types matches this function
    ///         signature with additional but supported implicit casting
    ///
    /// @param  input  A vector of llvm types representing the function argument types
    /// @param  C      The llvm context
    ///
    bool implicitMatch(std::vector<llvm::Type*>& input, llvm::LLVMContext& C) const;

    /// @brief  Returns true if this function has a return value which is not void
    ///
    /// @param  C  The llvm context
    ///
    bool hasReturnValue(llvm::LLVMContext& C) const;

    /// @brief  Returns true if this function has arguments which return a value
    ///
    inline bool hasOutputArguments() const {
        return mOutputArguments != 0;
    }

    /// @brief  Returns the number of return arguments. This includes whether the
    ///         function return type is non void.
    /// @param  C  The llvm context
    ///
    inline size_t numReturnValues(llvm::LLVMContext& C) const {
        return hasReturnValue(C) ? 1 + mOutputArguments : mOutputArguments;
    }

    /// @brief  Appends the output arguments to a vector of llvm values. This is useful
    ///         when constructing the correct llvm values to pass in as arguments due to
    ///         the front facing signatures requiring different output locations.
    /// @note   This does not include the function return value
    ///
    /// @param values   The vector of llvm values to append to
    /// @param builder  The current llvm IRBuilder
    ///
    void appendOutputArguments(std::vector<llvm::Value*>& values,
                       llvm::IRBuilder<>& builder) const;

    /// @brief  Appends the output argument types to a vector of llvm types.
    /// @note   This does not include the function return type
    ///
    /// @param values  The vector of llvm types to append to
    /// @param C       The llvm context
    ///
    void appendOutputTypes(std::vector<llvm::Type*>& types,
                      llvm::LLVMContext& C) const;

    /// @brief  Builds and registers this function signature and symbol name as an
    ///         available and callable function within the llvm module. Returns the
    ///         resulting llvm function
    ///
    /// @param M  The llvm module to insert the function into
    ///
    llvm::Function* toLLVMFunction(llvm::Module& M) const;

    /// @brief  Prints a description of this functions return type, name and signature
    ///
    /// @param C     The llvm context
    /// @param name  The name of the function to insert after it's return type
    /// @param os    The string stream to write to
    ///
    void print(llvm::LLVMContext& C, const std::string& name = "", std::ostream& os = std::cout) const;

protected:

    /// @brief  Base Function Signature constructor, expected to be instantiated by
    ///         derived classes.
    ///
    /// @param function  A void pointer to the available function if it is an external
    ///                  function
    /// @param symbol    The function symbol name. This must be the name of the function
    ///                  signature declaration if the function is external
    /// @param outArgs   The number of output arguments this function signature contains.
    ///                  This starts from the end of the function signature
    ///
    FunctionSignatureBase(void* const function,
                          const std::string& symbol,
                          const size_t outArgs = 0);

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

private:

    /// @brief  Returns true if an explicit match exists between two llvm type vectors
    ///
    /// @param input      The first vector of llvm types
    /// @param signature  The second vector of llvm types
    ///
    bool explicitMatch(const std::vector<llvm::Type*>& input,
                  const std::vector<llvm::Type*>& signature) const;

    /// @brief  Returns true if an implicit match exists between two llvm type vectors
    ///
    /// @param input      The first vector of llvm types
    /// @param signature  The second vector of llvm types
    ///
    bool implicitMatch(const std::vector<llvm::Type*>& input,
                  const std::vector<llvm::Type*>& signature) const;

protected:
    void* const mFunction;
    const std::string mSymbolName;
    const size_t mOutputArguments;
};

/// @brief  The base definition for a single function type.
///
struct FunctionBase
{
    using Ptr = std::shared_ptr<FunctionBase>;
    using FunctionList = std::vector<FunctionSignatureBase::Ptr>;
    using FunctionMatch =
        std::pair<FunctionSignatureBase::Ptr, FunctionSignatureBase::SignatureMatch>;

    /// @brief  An enum used to describe what context a function can be called in.
    ///         As some functions are coupled to the object they are called form, this
    ///         value is used to check the appropriate arguments are available.
    ///
    enum Context
    {
        Base = 0x1,
        Point = 0x2,
        Volume = 0x4,
        All = Base | Point | Volume
    };

    /// @brief  Pure virtual function which returns the function identifier for
    ///         this function type.
    ///
    virtual const std::string identifier() const = 0;

    /// @brief  Pure virtual function which returns the function context for
    ///         this function type.
    ///
    virtual FunctionBase::Context context() const = 0;

    /// @brief  Populate a vector of strings with the identifiers of any functions which
    ///         this function depends on. This is used by the compiler to ensure global
    ///         mappings are added for requires methods
    ///
    /// @param  identifiers  A vector of function identifiers to populate
    ///
    inline virtual void getDependencies(std::vector<std::string>& identifiers) const {}

    /// @brief  Populate a string with any documentation for this function
    ///
    /// @param  A string to populate
    ///
    inline virtual void getDocumentation(std::string& doc) const {}

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
    /// @param M            The llvm module
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
            llvm::Module& M,
            std::vector<llvm::Value*>* results = nullptr,
            const bool addOutputArguments = true) const;

    /// @brief  Generates the function body. Returns the function return value or a null
    ///         pointer if there is no function body (external function)
    ///
    /// @param args     A vector of llvm types representing the function argument values
    /// @param globals  The map of global names to llvm::Values
    /// @param builder  The current llvm IRBuilder
    /// @param M        The llvm Module to allow insertion of any additional functions
    ///                 (such as intrinsics)
    ///
    virtual llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
             const std::unordered_map<std::string, llvm::Value*>& globals,
             llvm::IRBuilder<>& builder,
             llvm::Module& M) const {
        return nullptr;
    }

    /// @brief  Accessor to the underlying function signature list
    ///
    inline const FunctionList& list() const { return mFunctionList; }

protected:

    /// @brief  Function Base constructor, expected to be instantiated by derived classes
    ///
    /// @param  A vector of function signatures corresponding to this function type
    ///
    FunctionBase(const FunctionList& list)
        : mFunctionList(list) {}
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

    FunctionSignature(void* const ptr, const std::string& symbol, const size_t outArgs = 0)
        : FunctionSignatureBase(ptr, symbol, outArgs) {
            if (outArgs > this->size()) {
                OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                    "\" has been constructed with " + std::to_string(outArgs) + " output arguments, "
                    "with only " + std::to_string(this->size()) + " available in signature.");
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
    /// @param outArgs   The number of output arguments this function has.
    ///
    static Ptr create(Signature function, const std::string& symbol, const size_t outArgs = 0) {

        void* functionVoidPtr = nullptr;

        if (function) {
            using FunctionPtrT = typename std::add_pointer<SignatureT>::type;
            FunctionPtrT* ptr = function.template target<FunctionPtrT>();
            if (!ptr) {
                OPENVDB_THROW(LLVMFunctionError, "Function object with symbol name \"" + symbol +
                    "\" does not match expected signature during creation.");
            }
            functionVoidPtr = reinterpret_cast<void*>(*ptr);
        }

        return Ptr(new FunctionSignatureT(functionVoidPtr, symbol, outArgs));
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
        ::create(&FunctionPtr, std::string(#FunctionPtr), 0)

#define DECLARE_FUNCTION_SIGNATURE_OUTPUT(FunctionPtr, ReturnIdx) \
    openvdb::ax::codegen::FunctionSignature<decltype(FunctionPtr)>\
        ::create(&FunctionPtr, std::string(#FunctionPtr), ReturnIdx)

#endif // OPENVDB_AX_CODEGEN_FUNCTION_TYPES_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
