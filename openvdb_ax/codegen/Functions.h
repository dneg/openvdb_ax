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

/// @file codegen/Functions.h
///
/// @authors Nick Avramoussis, Richard Jones
///
/// @brief  Contains the function objects that define the functions used in
///         compute function generation, to be inserted into the FunctionRegistry.
///         These define general purpose functions such as math functions.
///
///

#ifndef OPENVDB_AX_CODEGEN_GENERIC_FUNCTIONS_HAS_BEEN_INCLUDED
#define OPENVDB_AX_CODEGEN_GENERIC_FUNCTIONS_HAS_BEEN_INCLUDED

#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/codegen/FunctionTypes.h>
#include <openvdb_ax/codegen/Types.h>
#include <openvdb_ax/codegen/Utils.h>
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

// Macro for defining the function identifier and context. All functions must
// define these values

#define DEFINE_IDENTIFIER_CONTEXT_DOC(Identifier, CodeGenContext, Documentation) \
    inline FunctionBase::Context context() const override final { return CodeGenContext; } \
    inline const std::string identifier() const override final { return std::string(Identifier); } \
    inline void getDocumentation(std::string& doc) const override final { doc = Documentation; }

// LLVM Intrinsics which work on FP types

#define DEFINE_LLVM_FP_INTRINSIC(ClassName, Identifier, ASTToken, LLVMToken, STDFunc, Doc) \
    struct ClassName : public FunctionBase { \
        DEFINE_IDENTIFIER_CONTEXT_DOC(Identifier, FunctionBase::All, Doc) \
        inline static Ptr create(const FunctionOptions& op) { \
            return Ptr(op.mPrioritiseFunctionIR ? new ClassName(PrioritiseIRGeneration()) : new ClassName()); \
        }\
        ClassName() : FunctionBase({ \
                FunctionSignature<double(double)>::create\
                    ((double(*)(double))(STDFunc), std::string(#STDFunc"d"), 0), \
                FunctionSignature<float(float)>::create\
                    ((float(*)(float))(STDFunc), std::string(#STDFunc"f"), 0) \
            }) {} \
        ClassName(const PrioritiseIRGeneration&) : FunctionBase({ \
                FunctionSignature<double(double)>::create\
                    (nullptr, std::string(#Identifier"d"), 0), \
                FunctionSignature<float(float)>::create\
                    (nullptr, std::string(#Identifier"f"), 0) \
            }) {} \
        llvm::Value* \
        generate(const std::vector<llvm::Value*>& args, \
             const std::unordered_map<std::string, llvm::Value*>& globals, \
             llvm::IRBuilder<>& builder, \
             llvm::Module& M) const override final { \
            llvm::Function* function = llvm::Intrinsic::getDeclaration(&M, LLVMToken, args[0]->getType()); \
            return builder.CreateCall(function, args); \
        } \
    };
DEFINE_LLVM_FP_INTRINSIC(Sqrt, "sqrt", ast::tokens::SQRT, llvm::Intrinsic::sqrt, std::sqrt,
    "Computes the square root of arg.");
DEFINE_LLVM_FP_INTRINSIC(Sin, "sin", ast::tokens::SIN, llvm::Intrinsic::sin, std::sin,
    "Computes the sine of arg (measured in radians).");
DEFINE_LLVM_FP_INTRINSIC(Cos, "cos", ast::tokens::COS, llvm::Intrinsic::cos, std::cos,
    "Computes the cosine of arg (measured in radians).");
DEFINE_LLVM_FP_INTRINSIC(Log, "log", ast::tokens::LOG, llvm::Intrinsic::log, std::log,
    "Computes the natural (base e) logarithm of arg.");
DEFINE_LLVM_FP_INTRINSIC(Log10, "log10", ast::tokens::LOG10, llvm::Intrinsic::log10, std::log10,
    "Computes the common (base-10) logarithm of arg.");
DEFINE_LLVM_FP_INTRINSIC(Log2, "log2", ast::tokens::LOG2, llvm::Intrinsic::log2, std::log2,
    "Computes the binary (base-2) logarithm of arg.");
DEFINE_LLVM_FP_INTRINSIC(Exp, "exp", ast::tokens::EXP, llvm::Intrinsic::exp, std::exp,
    "Computes e (Euler's number, 2.7182818...) raised to the given power arg.");
DEFINE_LLVM_FP_INTRINSIC(Exp2, "exp2", ast::tokens::EXP2, llvm::Intrinsic::exp2, std::exp2,
    "Computes 2 raised to the given power arg.");
DEFINE_LLVM_FP_INTRINSIC(Fabs, "fabs", ast::tokens::FABS, llvm::Intrinsic::fabs, std::fabs,
    "Computes the absolute value of a floating point value arg.");
DEFINE_LLVM_FP_INTRINSIC(Floor, "floor", ast::tokens::FLOOR, llvm::Intrinsic::floor, std::floor,
    "Computes the largest integer value not greater than arg.");
DEFINE_LLVM_FP_INTRINSIC(Ceil, "ceil", ast::tokens::CEIL, llvm::Intrinsic::ceil, std::ceil,
    "Computes the smallest integer value not less than arg.");
DEFINE_LLVM_FP_INTRINSIC(Round, "round", ast::tokens::ROUND, llvm::Intrinsic::round, std::round,
    "Computes the nearest integer value to arg (in floating-point format), rounding halfway cases away "
    "from zero.");

// pow created explicitly as it takes two arguments and performs slighlty different
// calls for integer exponents

struct Pow : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("pow", FunctionBase::All,
        "Computes the value of the first argument raised to the power of the second argument.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Pow()); }

    Pow() : FunctionBase({
        FunctionSignature<double(double,double)>::create
            (nullptr, std::string("powd"), 0),
        FunctionSignature<float(float,float)>::create
            (nullptr, std::string("powf"), 0),
        FunctionSignature<double(double,int32_t)>::create
            (nullptr, std::string("powdi"), 0),
        FunctionSignature<float(float,int32_t)>::create
            (nullptr, std::string("powfi"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        llvm::Type* overloadType = args[0]->getType();
        llvm::Type* expType = args[1]->getType();

        const llvm::Intrinsic::ID id =
            expType->isIntegerTy() ? llvm::Intrinsic::powi : llvm::Intrinsic::pow;

        llvm::Function* function = llvm::Intrinsic::getDeclaration(&M, id, overloadType);
        return builder.CreateCall(function, args);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////


/// Internally supported function definitions

struct Acos : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("acos", FunctionBase::All,
        "Computes the principal value of the arc cosine of the input.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Acos()); }

    Acos() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::acos), std::string("acosd")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::acos), std::string("acosf"))
    }) {}
};

struct Asin : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("asin", FunctionBase::All,
        "Computes the principal value of the arc sine of the input.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Asin()); }

    Asin() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::asin), std::string("asind")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::asin), std::string("asinf"))
    }) {}
};

struct Atan : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("atan", FunctionBase::All,
        "Computes the principal value of the arc tangent of the input.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atan()); }

    Atan() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::atan), std::string("atand")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::atan), std::string("atanf"))
    }) {}
};

struct Atan2 : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("atan2", FunctionBase::All,
        "Computes the arc tangent of y/x using the signs of arguments to determine "
        "the correct quadrant.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atan2()); }

    Atan2() : FunctionBase({
        FunctionSignature<double(double,double)>::create
            ((double(*)(double,double))(std::atan2), std::string("atan2d")),
        FunctionSignature<float(float,float)>::create
            ((float(*)(float,float))(std::atan2), std::string("atan2f"))
    }) {}
};

struct Cbrt : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("cbrt", FunctionBase::All,
        "Computes the cubic root of the input.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Cbrt()); }

    Cbrt() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::cbrt), std::string("cbrtd")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::cbrt), std::string("cbrtf"))
    }) {}
};

struct Sinh : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("sinh", FunctionBase::All,
        "Computes the hyperbolic sine of the input")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Sinh()); }

    Sinh() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::sinh), std::string("sinhd")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::sinh), std::string("sinhf"))
    }) {}
};

struct Cosh : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("cosh", FunctionBase::All,
        "Computes the hyperbolic cosine of the input")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Cosh()); }

    Cosh() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::cosh), std::string("coshd")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::cosh), std::string("coshf"))
    }) {}
};

struct Tanh : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("tanh", FunctionBase::All,
        "Computes the hyperbolic tangent of the input")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Tanh()); }

    Tanh() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::tanh), std::string("tanhd")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::tanh), std::string("tanhf"))
    }) {}
};

struct Rand : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("rand", FunctionBase::All,
        "Creates a random number based on the provided seed. The number will be "
        "in the range of 0 to 1. The same number is produced for the same seed.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Rand()); }

    Rand() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(rand_double),
            DECLARE_FUNCTION_SIGNATURE(rand_int)
    }) {}

private:

    static double rand_double(double seed);

    static double rand_int(int32_t seed);
};

struct Print : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("print", FunctionBase::All,
        "Prints the input to the standard error stream. Warning: This will be run for every "
        "element.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Print()); }

    Print() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(print<double>),
        DECLARE_FUNCTION_SIGNATURE(print<int32_t>),
        DECLARE_FUNCTION_SIGNATURE(print_string)
    }) {}

private:
    template <typename T>
    inline static void print(const T v) { std::cerr << v << std::endl; }
    inline static void print_string(StringPtrType array) {
        const char * const sarray = reinterpret_cast<const char* const>(array);
        std::cerr << sarray << std::endl;
    }
};

struct Atoi : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("atoi", FunctionBase::All,
        "Parses the string input interpreting its content as an integral number, which is "
        "returned as a value of type int.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atoi()); }

    Atoi() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(std::atoi),
        DECLARE_FUNCTION_SIGNATURE(std::atol)
    }) {}
};

struct Atof : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("atof", FunctionBase::All,
        "Parses the string input, interpreting its content as a floating point number and "
        "returns its value as a double.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atof()); }

    Atof() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(std::atof)
    }) {}
};

struct Signbit : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("signbit", FunctionBase::All,
        "Determines if the given floating point number input is negative.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Signbit()); }

    Signbit() : FunctionBase({
        FunctionSignature<bool(double)>::create
            ((bool(*)(double))(std::signbit), std::string("signbitd")),
        FunctionSignature<bool(float)>::create
            ((bool(*)(float))(std::signbit), std::string("signbitf"))
    }) {}
};

struct Abs : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("abs", FunctionBase::All,
        "Computes the absolute value of an integer number.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Abs(PrioritiseIRGeneration()) : new Abs());
    }

    Abs() : FunctionBase({
        FunctionSignature<int32_t(int32_t)>::create
            ((int32_t(*)(int32_t))(std::abs), std::string("absi")),
        FunctionSignature<long(long)>::create
            ((long(*)(long))(std::abs), std::string("absl"))
    }) {}

    Abs(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<int32_t(int32_t)>::create(nullptr, std::string("absi"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {
        // arithmetic shift right
        llvm::Value* mask = builder.CreateAShr(args[0], LLVMType<int32_t>::get(builder.getContext(), 31));
        llvm::Value* xorResult = binaryOperator(args[0], mask, ast::tokens::BITXOR, builder);
        return binaryOperator(xorResult, mask, ast::tokens::MINUS, builder);
    }
};

struct Lookup : public FunctionBase
{
protected:
    template <typename T>
    inline static T lookup(const uint8_t* const name, const void* const data) {
        const ax::CustomData* const customData =
            static_cast<const ax::CustomData* const>(data);
        const std::string nameStr(reinterpret_cast<const char* const>(name));

        const TypedMetadata<T>* const metaData =
            customData->getData<TypedMetadata<T>>(nameStr);
        return metaData ? metaData->value() : zeroVal<T>();
    }

    template <typename T>
    inline static void lookupvec3(const uint8_t* const name,
                                    const void* const data,
                                    T (*out)[3]) {
        const math::Vec3<T> result =
            lookup<math::Vec3<T>>(name, data);
        for (size_t i = 0; i < 3; ++i) (*out)[i] = result[i];
    }

    Lookup(const FunctionBase::FunctionList& list)
        : FunctionBase(list) {}
};


struct LookupFloat : public Lookup
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_lookupf", FunctionBase::All,
            "Internal function for looking up a custom float value.")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(Lookup::lookup<float>)
        }) {}
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("lookupf", FunctionBase::All,
        "Find a custom user parameter with a given name of type 'float' in the Custom data "
        "provided to the AX compiler. If the data can not be found, or is not of the expected type "
        "0.0f is returned.")
    inline static FunctionBase::Ptr create(const FunctionOptions&) { return Ptr(new LookupFloat()); }

    LookupFloat() : Lookup({
        FunctionSignature<float(const uint8_t* const)>::create
            (nullptr, std::string("lookupf"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_lookupf");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override {

        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(globals.at("custom_data"));

        Internal func;
        return func.execute(internalArgs, globals, builder, M);
     }
};

struct LookupVec3f : public Lookup
{

    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_lookupvec3f", FunctionBase::All,
            "Internal function for looking up a custom vector float value.")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(Lookup::lookupvec3<float>, 1)
        }) {}
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("lookupvf", FunctionBase::All,
        "Find a custom user parameter with a given name of type 'vector float' in the Custom data "
        "provided to the AX compiler. If the data can not be found, or is not of the expected type "
        "{ 0.0f, 0.0f, 0.0f } is returned.")
    inline static FunctionBase::Ptr create(const FunctionOptions&) { return Ptr(new LookupVec3f()); }

    LookupVec3f() : Lookup({
        FunctionSignature<V3F*(const uint8_t* const)>::create
            (nullptr, std::string("lookupvec3f"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_lookupvec3f");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override {

        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(globals.at("custom_data"));

        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(internalArgs, globals, builder, M, &results);

        assert(!results.empty());
        return results.front();
     }
};

struct Min : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("min", FunctionBase::All,
        "Returns the smaller of the given values.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Min(PrioritiseIRGeneration()) : new Min());
    }

    Min() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(Min::min<double>),
        DECLARE_FUNCTION_SIGNATURE(Min::min<float>),
        DECLARE_FUNCTION_SIGNATURE(Min::min<int32_t>)
    }) {}

    // @todo  Investigate reference support
    // Min() : FunctionBase({
    //     FunctionSignature<const double&(const double&,const double&)>::create
    //         ((const double&(*)(const double&, const double&))(std::min), std::string("min"), 0),
    //     FunctionSignature<const float&(const float&,const float&)>::create
    //         ((const float&(*)(const float&, const float&))(std::min), std::string("min"), 0),
    //     FunctionSignature<const int&(const int&,const int&)>::create
    //         ((const int&(*)(const int&, const int&))(std::min), std::string("min"), 0)
    // }) {}

    Min(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<double(double,double)>::create
            (nullptr, std::string("mind")),
        FunctionSignature<float(float,float)>::create
            (nullptr, std::string("minf")),
        FunctionSignature<int32_t(int32_t,int32_t)>::create
            (nullptr, std::string("mini"))
    }) {}

    inline llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        llvm::Value* result =
            binaryOperator(args[0], args[1], ast::tokens::MORETHAN, builder);
        return builder.CreateSelect(result, args[1], args[0], "mintmp");
    }

private:
    template<typename T>
    inline static T min(T a, T b) { return std::min(a,b); }
};

struct Max : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("max", FunctionBase::All,
        "Returns the larger of the given values.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Max(PrioritiseIRGeneration()) : new Max());
    }

    Max() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(Max::max<double>),
        DECLARE_FUNCTION_SIGNATURE(Max::max<float>),
        DECLARE_FUNCTION_SIGNATURE(Max::max<int32_t>)
    }) {}

    // @todo  Investigate reference support
    // Max() : FunctionBase({
    //     FunctionSignature<const double&(const double&,const double&)>::create
    //         ((const double&(*)(const double&, const double&))(std::max), std::string("max"), 0),
    //     FunctionSignature<const float&(const float&,const float&)>::create
    //         ((const float&(*)(const float&, const float&))(std::max), std::string("max"), 0),
    //     FunctionSignature<const int&(const int&,const int&)>::create
    //         ((const int&(*)(const int&, const int&))(std::max), std::string("max"), 0)
    // }) {}

    Max(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<double(double,double)>::create
            (nullptr, std::string("maxd")),
        FunctionSignature<float(float,float)>::create
            (nullptr, std::string("maxf")),
        FunctionSignature<int32_t(int32_t,int32_t)>::create
            (nullptr, std::string("maxi"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        llvm::Value* result =
            binaryOperator(args[0], args[1], ast::tokens::MORETHAN, builder);
        return builder.CreateSelect(result, args[0], args[1], "maxtmp");
    }

private:
    template<typename T>
    inline static T max(T a, T b) { return std::max(a,b); }
};

struct Tan : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("tan", FunctionBase::All,
        "Computes the tangent of arg (measured in radians).")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Tan(PrioritiseIRGeneration()) : new Tan());
    }

    Tan() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(std::tan), std::string("tand")),
        FunctionSignature<float(float)>::create
            ((float(*)(float))(std::tan), std::string("tanf")),
        FunctionSignature<double(int32_t)>::create
            ((double(*)(int32_t))(std::tan), std::string("tani"))
    }) {}

    Tan(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<double(double)>::create
            (nullptr, std::string("tand")),
        FunctionSignature<float(float)>::create
            (nullptr, std::string("tanf"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        llvm::Type* type = args[0]->getType();
        llvm::Function* sinFunction =
            llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::sin, type);
        llvm::Function* cosFunction =
            llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::cos, type);

        llvm::Value* sin = builder.CreateCall(sinFunction, args[0], "sintmp");
        llvm::Value* cos = builder.CreateCall(cosFunction, args[0], "costmp");
        return binaryOperator(sin, cos, ast::tokens::DIVIDE, builder);
    }
};

struct LengthSq : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("lengthsq", FunctionBase::All,
        "Returns the squared length of the given vector")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new LengthSq(PrioritiseIRGeneration()) : new LengthSq());
    }

    LengthSq() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(LengthSq::lensq<double>),
        DECLARE_FUNCTION_SIGNATURE(LengthSq::lensq<float>),
        DECLARE_FUNCTION_SIGNATURE(LengthSq::lensq<int32_t>)
    }) {}

    LengthSq(const PrioritiseIRGeneration&) : FunctionBase({
            FunctionSignature<double(double (*)[3])>::create
                (nullptr, std::string("lensqd")),
            FunctionSignature<float(float (*)[3])>::create
                (nullptr, std::string("lensqf")),
            FunctionSignature<int32_t(int32_t (*)[3])>::create
                (nullptr, std::string("lensqi"))
        }) {}

    inline static llvm::Value*
    doGenerate(llvm::Value* ptrToVector,
               std::vector<llvm::Value*>& elements,
               llvm::IRBuilder<>& builder)
    {
        arrayUnpack(ptrToVector, elements, builder, /*load*/true);
        assert(elements.size() == 3);

        llvm::Value* v1 = binaryOperator(elements[0], elements[0], ast::tokens::MULTIPLY, builder);
        llvm::Value* v2 = binaryOperator(elements[1], elements[1], ast::tokens::MULTIPLY, builder);
        llvm::Value* v3 = binaryOperator(elements[2], elements[2], ast::tokens::MULTIPLY, builder);
        llvm::Value* result = binaryOperator(v1, v2, ast::tokens::PLUS, builder);
        return binaryOperator(result, v3, ast::tokens::PLUS, builder);
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        std::vector<llvm::Value*> elements;
        return doGenerate(args[0], elements, builder);
    }

private:
    template <typename T>
    inline static T lensq(T (*in)[3]) {
        return openvdb::math::Vec3<T>(in[0]).lengthSqr();
    }
};

struct Length : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("length", FunctionBase::All,
        "Returns the length of the given vector")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Length(PrioritiseIRGeneration()) : new Length());
    }

    Length() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(Length::length<double>),
        DECLARE_FUNCTION_SIGNATURE(Length::length<float>)
    }) {}

    // @note  allow explicit int signature for initial length computing optimization
    Length(const PrioritiseIRGeneration&) : FunctionBase({
            FunctionSignature<double(double (*)[3])>::create
                (nullptr, std::string("lengthd")),
            FunctionSignature<float(float (*)[3])>::create
                (nullptr, std::string("lengthf")),
            FunctionSignature<float(int32_t (*)[3])>::create
                (nullptr, std::string("lengthi"))
        }) {}

    inline static llvm::Value*
    doGenerate(llvm::Value* ptrToVector,
           std::vector<llvm::Value*>& elements,
           llvm::IRBuilder<>& builder,
           llvm::Module& M)
    {
        llvm::Value* result = LengthSq::doGenerate(ptrToVector, elements, builder);

        llvm::Type* floatT = LLVMType<float>::get(builder.getContext());
        if (result->getType()->isIntegerTy()) {
           result = arithmeticConversion(result, floatT, builder);
        }

        llvm::Function* sqrt =
            llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::sqrt, result->getType());
        return builder.CreateCall(sqrt, result, "lengthsqrt");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        std::vector<llvm::Value*> elements;
        return doGenerate(args[0], elements, builder, M);
    }

private:
    template <typename T>
    inline static T length(T (*in)[3]) {
        return openvdb::math::Vec3<T>(in[0]).length();
    }
};

struct Normalize : public FunctionBase
{
    struct Internal : public FunctionBase
    {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_normalize", FunctionBase::All,
            "Internal function for performing vector normalization")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(normalize<double>, 1),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(normalize<float>, 1)
        }) {}

        private:
            template <typename T>
            inline static void normalize(T (*in)[3], T (*out)[3]) {
                openvdb::math::Vec3<T> inv(in[0]); inv.normalize();
                for (size_t i = 0; i < 3; ++i) out[0][i] = inv[i];
            }
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("normalize", FunctionBase::All,
        "Returns the normalized result of the given vector.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Normalize(PrioritiseIRGeneration()) : new Normalize());
    }

    Normalize() : FunctionBase({
        FunctionSignature<V3D*(V3D*)>::create
            (nullptr, std::string("normalized"), 0),
        FunctionSignature<V3F*(V3F*)>::create
            (nullptr, std::string("normalizef"), 0)
    }), mExternalCall(true) {}

    // @note  allow explicit int signature for initial length computing optimization
    Normalize(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<V3D*(V3D*)>::create
            (nullptr, std::string("normalized"), 0),
        FunctionSignature<V3F*(V3F*)>::create
            (nullptr, std::string("normalizef"), 0),
        FunctionSignature<V3I*(V3I*)>::create
            (nullptr, std::string("normalizei"), 0)
    }), mExternalCall(false) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        if (mExternalCall) identifiers.emplace_back("internal_normalize");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        llvm::Value* result = nullptr;

        if (mExternalCall) {
            std::vector<llvm::Value*> results;
            Internal func;
            func.execute(args, globals, builder, M, &results);
            assert(!results.empty());
            result = results.front();
        }
        else {
            std::vector<llvm::Value*> elements;
            llvm::Value* lSq = Length::doGenerate(args.front(), elements, builder, M);

            if (elements[0]->getType()->isIntegerTy()) {
               llvm::Type* floatT = LLVMType<float>::get(builder.getContext());
               elements[0] = arithmeticConversion(elements[0], floatT, builder);
               elements[1] = arithmeticConversion(elements[1], floatT, builder);
               elements[2] = arithmeticConversion(elements[2], floatT, builder);
            }

            // the following is always done at fp precision

            llvm::Value* one = llvm::ConstantFP::get(lSq->getType(), 1.0);
            llvm::Value* oneDividedByLength = builder.CreateFDiv(one, lSq);

            elements[0] = builder.CreateFMul(elements[0], oneDividedByLength);
            elements[1] = builder.CreateFMul(elements[1], oneDividedByLength);
            elements[2] = builder.CreateFMul(elements[2], oneDividedByLength);

            // pack and store in second argument
            result = arrayPack(elements, builder);
        }

        return result;
    }

private:
    const bool mExternalCall;
};

struct DotProd : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("dot", FunctionBase::All,
        "Computes the dot product of two vectors")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new DotProd(PrioritiseIRGeneration()) : new DotProd());
    }

    DotProd() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(dot<double>),
        DECLARE_FUNCTION_SIGNATURE(dot<float>),
        DECLARE_FUNCTION_SIGNATURE(dot<int32_t>)
    }) {}

    DotProd(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<double(double (*)[3], double (*)[3])>::create
            (nullptr, std::string("dotprodd")),
        FunctionSignature<float(float (*)[3], float (*)[3])>::create
            (nullptr, std::string("dotprodf")),
        FunctionSignature<int32_t(int32_t (*)[3], int32_t (*)[3])>::create
            (nullptr, std::string("dotprodi"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        std::vector<llvm::Value*> v1, v2;
        arrayUnpack(args[0], v1, builder, /*load*/true);
        arrayUnpack(args[1], v2, builder, /*load*/true);

        v1[0] = binaryOperator(v1[0], v2[0], ast::tokens::MULTIPLY, builder);
        v1[1] = binaryOperator(v1[1], v2[1], ast::tokens::MULTIPLY, builder);
        v1[2] = binaryOperator(v1[2], v2[2], ast::tokens::MULTIPLY, builder);

        llvm::Value* result = binaryOperator(v1[0], v1[1], ast::tokens::PLUS, builder);
        result = binaryOperator(result, v1[2], ast::tokens::PLUS, builder);

        return result;
    }
private:
    template <typename T>
    inline static T dot(T (*in1)[3], T (*in2)[3]) {
        const openvdb::math::Vec3<T> inv1(*in1);
        const openvdb::math::Vec3<T> inv2(*in2);
        return inv1.dot(inv2);
    }
};

struct CrossProd : public FunctionBase
{
    struct Internal : public FunctionBase
    {
        DEFINE_IDENTIFIER_CONTEXT_DOC("internal_cross", FunctionBase::All,
            "Internal function for performing vector cross products")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<double>, 1),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<float>, 1),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<int32_t>, 1)
        }) {}

    private:
        template <typename T>
        inline static void cross(T (*in1)[3], T (*in2)[3], T (*out)[3]) {
            const openvdb::math::Vec3<T> inv1(*in1);
            const openvdb::math::Vec3<T> inv2(*in2);
            const openvdb::math::Vec3<T> result = inv1.cross(inv2);
            for (size_t i = 0; i < 3; ++i) out[0][i] = result[i];
        }
    };

    DEFINE_IDENTIFIER_CONTEXT_DOC("cross", FunctionBase::All,
        "Computes the cross product of two vectors")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new CrossProd()); }

    CrossProd() : FunctionBase({
        FunctionSignature<V3D*(V3D*,V3D*)>::create
            (nullptr, std::string("crossprodd"), 0),
        FunctionSignature<V3F*(V3F*,V3F*)>::create
            (nullptr, std::string("crossprodf"), 0),
        FunctionSignature<V3I*(V3I*,V3I*)>::create
            (nullptr, std::string("crossprodi"), 0)
    }) {}

    inline void getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_cross");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final {

        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(args, globals, builder, M, &results);
        assert(!results.empty());

        return results.front();
    }
};

struct Clamp : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("clamp", FunctionBase::All,
        "Clamps the first argument to the minimum second argument value and maximum third "
        "argument value")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ? new Clamp(PrioritiseIRGeneration()) : new Clamp());
    }

    Clamp() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(openvdb::math::Clamp<double>),
        DECLARE_FUNCTION_SIGNATURE(openvdb::math::Clamp<float>),
        DECLARE_FUNCTION_SIGNATURE(openvdb::math::Clamp<int32_t>)
    }) {}

    Clamp(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<double(double, double, double)>::create
            (nullptr, std::string("clampd")),
        FunctionSignature<double(float, float, float)>::create
            (nullptr, std::string("clampf")),
        FunctionSignature<double(int32_t, int32_t, int32_t)>::create
            (nullptr, std::string("clampi"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override
    {
        // Arg format as follows:
        //
        //   args[0] = value
        //   args[1] = start
        //   args[2] = end
        //
        // I'm currently doing the following to avoid branching
        //
        //   temp = (value + end - abs(value-end))
        //   temp2 = (2.0*start) + abs(temp-(2.0*start))
        //   result = (temp + temp2) * 0.25;
        //
        // @NOTE  The multiplcation by 0.25 requires a cast for ints! For which
        //        everything gets converted to double precision

        auto& context = builder.getContext();

        llvm::Type* type = args[0]->getType();
        const bool isInt = type->isIntegerTy();

        llvm::Value* temp = binaryOperator(args[0], args[2], ast::tokens::MINUS, builder);

        // We have to use the IR implementation of ABS as ABS is not currently registered
        // if not used explicitly! @TODO!
        Abs absFunc = Abs(PrioritiseIRGeneration());

        if (isInt) temp = absFunc.execute({temp}, globals, builder, M);
        else {
            llvm::Function* abs =
                llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::fabs, type);
            temp = builder.CreateCall(abs, temp, "abs");
        }

        temp = binaryOperator(args[2], temp, ast::tokens::MINUS, builder);
        temp = binaryOperator(args[0], temp, ast::tokens::PLUS, builder);

        llvm::Value* two = isInt ? LLVMType<int32_t>::get(context, 2) : llvm::ConstantFP::get(type, 2.0);
        llvm::Value* startMultTwo = binaryOperator(two, args[1], ast::tokens::MULTIPLY, builder);

        llvm::Value* sub = binaryOperator(temp, startMultTwo, ast::tokens::MINUS, builder);

        llvm::Value* result = nullptr;
        if (isInt) result = absFunc.execute({sub}, globals, builder, M);
        else {
            llvm::Function* abs =
                llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::fabs, type);
            result = builder.CreateCall(abs, sub, "abs");
        }

        result = binaryOperator(startMultTwo, result, ast::tokens::PLUS, builder);
        result = binaryOperator(temp, result, ast::tokens::PLUS, builder);

        llvm::Type* doubleT = LLVMType<double>::get(context);
        result = arithmeticConversion(result, doubleT, builder);

        llvm::Value* pointTwoFive = LLVMType<double>::get(context, 0.25);
        return binaryOperator(result, pointTwoFive, ast::tokens::MULTIPLY, builder);
    }
};


struct Fit : public FunctionBase
{
    DEFINE_IDENTIFIER_CONTEXT_DOC("fit", FunctionBase::All,
        "Fit the first argument to the output range by first clamping the value between the second and "
        "third input range arguments and then remapping the result to the output range fourth and fifth "
        "arguments")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Fit()); }

    Fit() : FunctionBase({
        FunctionSignature<double(double, double, double, double, double)>::create
            (nullptr, std::string("fitd")),
        FunctionSignature<float(float, float, float, float, float)>::create
            (nullptr, std::string("fitf")),
        FunctionSignature<double(int32_t, int32_t, int32_t, int32_t, int32_t)>::create
            (nullptr, std::string("fiti"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& builder,
         llvm::Module& M) const override final
    {
        /*
                (outMax - outMin)(x - inMin)
        f(x) = ----------------------------  + outMin
                       inMax - inMin

        if inMax == inMin, f(x) = (outMax + outMin) / 2.0

        */

        // NOTE: this also performs a clamp on the ordered input range
        // @TODO revisit. If this is the best thing to do, should add conditional branching
        // so that the clamping math is never executed when the value is inside

        // args[0] = x
        // args[1] = inMin
        // args[2] = inMax
        // args[3] = outMin
        // args[4] = outMax

        // See if the input range has a valid magnitude .i.e. the values are not the same

        std::vector<llvm::Value*> argcopy(args);

        llvm::Value* isInputRangeValid =
            binaryOperator(argcopy[1], argcopy[2], ast::tokens::NOTEQUALS, builder);

        // clamp the input to the ORDERED inMin to inMax range

        llvm::Value* minRangeComp =
            binaryOperator(argcopy[1], argcopy[2], ast::tokens::LESSTHAN, builder);
        llvm::Value* minInputRange = builder.CreateSelect(minRangeComp, argcopy[1], argcopy[2]);
        llvm::Value* maxInputRange = builder.CreateSelect(minRangeComp, argcopy[2], argcopy[1]);

        // clamp
        {
            // We have to use the IR implementation of Clamp as Clamp is not currently registered
            // if not used explicitly! @TODO!
            Clamp clampOp = Clamp(PrioritiseIRGeneration());
            argcopy[0] = clampOp.execute({ argcopy[0], minInputRange, maxInputRange }, globals, builder, M);
        }

        // Cast argcopy to FP precision. Check arg[0] as if clamp has been called
        // with external functions it may be a float type (not double)

        llvm::Type* precision = LLVMType<double>::get(builder.getContext());
        if (!argcopy[0]->getType()->isIntegerTy()) precision = argcopy[0]->getType();
        assert(precision->isFloatingPointTy());

        for (auto& arg : argcopy) arg = arithmeticConversion(arg, precision, builder);

        llvm::Value* valueMinusMin = builder.CreateFSub(argcopy[0], argcopy[1]);
        llvm::Value* inputRange = builder.CreateFSub(argcopy[2], argcopy[1]);
        llvm::Value* outputRange = builder.CreateFSub(argcopy[4], argcopy[3]);

        llvm::Value* result = builder.CreateFMul(outputRange, valueMinusMin);
        result = builder.CreateFDiv(result, inputRange);  // NOTE - This can cause division by zero
        result = builder.CreateFAdd(argcopy[3], result);

        // calculate the output range over 2 and use this value if the input range is invalid

        llvm::Value* outputRangeOverTwo = builder.CreateFAdd(argcopy[3], argcopy[4]);
        llvm::Value* two = llvm::ConstantFP::get(precision, 2.0);
        outputRangeOverTwo = builder.CreateFDiv(outputRangeOverTwo, two);

        return builder.CreateSelect(isInputRangeValid, result, outputRangeOverTwo);
    }
};

}
}
}
}

#endif // OPENVDB_AX_CODEGEN_GENERIC_FUNCTIONS_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
