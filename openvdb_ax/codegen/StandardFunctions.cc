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

/// @file codegen/Functions.h
///
/// @authors Nick Avramoussis, Richard Jones, Francisco Gochez
///
/// @brief  Contains the function objects that define the functions used in
///         compute function generation, to be inserted into the FunctionRegistry.
///         These define general purpose functions such as math functions.
///

#include "Functions.h"

#include <openvdb_ax/math/OpenSimplexNoise.h>
#include <openvdb_ax/ast/Tokens.h>
#include <openvdb_ax/codegen/FunctionTypes.h>
#include <openvdb_ax/codegen/Types.h>
#include <openvdb_ax/codegen/Utils.h>
#include <openvdb_ax/compiler/CompilerOptions.h>
#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/version.h>

#include <boost/functional/hash.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>

#include <tbb/enumerable_thread_specific.h>
#include <unordered_map>

#include <stddef.h>
#include <stdint.h>

namespace {

// Reduce a size_t hash down into an unsigned int, taking all bits in the
// size_t into account. We achieve this by repeatedly XORing as many bytes
// that fit into an unsigned int, and then shift those bytes out of the
// hash. We repeat until we have no bits left in the hash.
template <typename SeedType>
inline SeedType hashToSeed(std::size_t hash) {
    SeedType seed = 0;
    do {
        seed ^= (SeedType) hash;
    } while (hash >>= sizeof(SeedType) * 8);
    return seed;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// Curl noise

template <typename NoiseT>
void curlnoise_vec3d(const double (*in)[3], double (*out)[3])
{
    float delta = 0.0001f;
    float a, b;

    // noise coordinates for vector potential components.
    float p[3][3] = {
        { static_cast<float>((*in)[0]) + 000.0f, static_cast<float>((*in)[1]) + 000.0f, static_cast<float>((*in)[2]) + 000.0f }, // x
        { static_cast<float>((*in)[0]) + 256.0f, static_cast<float>((*in)[1]) - 256.0f, static_cast<float>((*in)[2]) + 256.0f }, // y
        { static_cast<float>((*in)[0]) - 512.0f, static_cast<float>((*in)[1]) + 512.0f, static_cast<float>((*in)[2]) - 512.0f }, // z
    };

    OPENVDB_NO_TYPE_CONVERSION_WARNING_BEGIN
    // Compute curl.x
    a = (NoiseT::noise(p[2][0], p[2][1] + delta, p[2][2]) - NoiseT::noise(p[2][0], p[2][1] - delta, p[2][2])) / (2.0f * delta);
    b = (NoiseT::noise(p[1][0], p[1][1], p[1][2] + delta) - NoiseT::noise(p[1][0], p[1][1], p[1][2] - delta)) / (2.0f * delta);
    (*out)[0] = a - b;

    // Compute curl.y
    a = (NoiseT::noise(p[0][0], p[0][1], p[0][2] + delta) - NoiseT::noise(p[0][0], p[0][1], p[0][2] - delta)) / (2.0f * delta);
    b = (NoiseT::noise(p[2][0] + delta, p[2][1], p[2][2]) - NoiseT::noise(p[2][0] - delta, p[2][1], p[2][2])) / (2.0f * delta);
    (*out)[1] = a - b;

    // Compute curl.z
    a = (NoiseT::noise(p[1][0] + delta, p[1][1], p[1][2]) - NoiseT::noise(p[1][0] - delta, p[1][1], p[1][2])) / (2.0f * delta);
    b = (NoiseT::noise(p[0][0], p[0][1] + delta, p[0][2]) - NoiseT::noise(p[0][0], p[0][1] - delta, p[0][2])) / (2.0f * delta);
    (*out)[2] = a - b;
    OPENVDB_NO_TYPE_CONVERSION_WARNING_END
}

template <typename NoiseT>
void curlnoise_ddd(double x, double y, double z, double (*out)[3])
{
    const double in[3] = {x, y, z};
    curlnoise_vec3d<NoiseT>(&in, out);
}

}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

// Macro for defining the function identifier and context. All functions must
// define these values

#define DEFINE_IDENTIFIER_DOC(Identifier, Documentation) \
    inline const std::string identifier() const override final { return std::string(Identifier); } \
    inline void getDocumentation(std::string& doc) const override final { doc = Documentation; }

// LLVM Intrinsics which work on FP types

#define DEFINE_LLVM_FP_INTRINSIC(ClassName, Identifier, ASTToken, LLVMToken, STDFunc, Doc) \
    struct ClassName : public FunctionBase { \
        DEFINE_IDENTIFIER_DOC(Identifier, Doc) \
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
             const std::unordered_map<std::string, llvm::Value*>&, \
             llvm::IRBuilder<>& B) const override final { \
            llvm::Module* M = B.GetInsertBlock()->getParent()->getParent(); \
            llvm::Function* function = llvm::Intrinsic::getDeclaration(M, LLVMToken, args[0]->getType()); \
            return B.CreateCall(function, args); \
        } \
    };
DEFINE_LLVM_FP_INTRINSIC(Sqrt, "sqrt", ast::tokens::SQRT, llvm::Intrinsic::sqrt,
    std::sqrt, "Computes the square root of arg.")
DEFINE_LLVM_FP_INTRINSIC(Sin, "sin", ast::tokens::SIN, llvm::Intrinsic::sin,
    std::sin, "Computes the sine of arg (measured in radians).")
DEFINE_LLVM_FP_INTRINSIC(Cos, "cos", ast::tokens::COS, llvm::Intrinsic::cos,
    std::cos, "Computes the cosine of arg (measured in radians).")
DEFINE_LLVM_FP_INTRINSIC(Log, "log", ast::tokens::LOG, llvm::Intrinsic::log,
    std::log, "Computes the natural (base e) logarithm of arg.")
DEFINE_LLVM_FP_INTRINSIC(Log10, "log10", ast::tokens::LOG10, llvm::Intrinsic::log10,
    std::log10, "Computes the common (base-10) logarithm of arg.")
DEFINE_LLVM_FP_INTRINSIC(Log2, "log2", ast::tokens::LOG2, llvm::Intrinsic::log2,
    std::log2, "Computes the binary (base-2) logarithm of arg.")
DEFINE_LLVM_FP_INTRINSIC(Exp, "exp", ast::tokens::EXP, llvm::Intrinsic::exp,
    std::exp, "Computes e (Euler's number, 2.7182818...) raised to the given power arg.")
DEFINE_LLVM_FP_INTRINSIC(Exp2, "exp2", ast::tokens::EXP2, llvm::Intrinsic::exp2,
    std::exp2, "Computes 2 raised to the given power arg.")
DEFINE_LLVM_FP_INTRINSIC(Fabs, "fabs", ast::tokens::FABS, llvm::Intrinsic::fabs,
    std::fabs, "Computes the absolute value of a floating point value arg.")
DEFINE_LLVM_FP_INTRINSIC(Floor, "floor", ast::tokens::FLOOR, llvm::Intrinsic::floor,
    std::floor, "Computes the largest integer value not greater than arg.")
DEFINE_LLVM_FP_INTRINSIC(Ceil, "ceil", ast::tokens::CEIL, llvm::Intrinsic::ceil,
    std::ceil, "Computes the smallest integer value not less than arg.")
DEFINE_LLVM_FP_INTRINSIC(Round, "round", ast::tokens::ROUND, llvm::Intrinsic::round,
    std::round, "Computes the nearest integer value to arg (in floating-point format), "
    "rounding halfway cases away from zero.")

// pow created explicitly as it takes two arguments and performs slighlty different
// calls for integer exponents

struct Pow : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("pow", "Computes the value of the first argument "
        "raised to the power of the second argument.")

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
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        llvm::Type* overloadType = args[0]->getType();
        llvm::Type* expType = args[1]->getType();

        const llvm::Intrinsic::ID id =
            expType->isIntegerTy() ? llvm::Intrinsic::powi : llvm::Intrinsic::pow;

        llvm::Module* M = B.GetInsertBlock()->getParent()->getParent();
        llvm::Function* function = llvm::Intrinsic::getDeclaration(M, id, overloadType);
        return B.CreateCall(function, args);
    }
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


/// Internally supported function definitions

struct Acos : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("acos", "Computes the principal value of the arc "
        "cosine of the input.")

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
    DEFINE_IDENTIFIER_DOC("asin", "Computes the principal value of the arc sine "
        "of the input.")

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
    DEFINE_IDENTIFIER_DOC("atan", "Computes the principal value of the arc "
        "tangent of the input.")

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
    DEFINE_IDENTIFIER_DOC("atan2", "Computes the arc tangent of y/x using the "
        "signs of arguments to determine the correct quadrant.")

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
    DEFINE_IDENTIFIER_DOC("cbrt", "Computes the cubic root of the input.")

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
    DEFINE_IDENTIFIER_DOC("sinh", "Computes the hyperbolic sine of the input")

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
    DEFINE_IDENTIFIER_DOC("cosh", "Computes the hyperbolic cosine of the input")

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
    DEFINE_IDENTIFIER_DOC("tanh", "Computes the hyperbolic tangent of the input")

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
    DEFINE_IDENTIFIER_DOC("rand", "Creates a random number based on the provided "
        "seed. The number will be in the range of 0 to 1. The same number is "
        "produced for the same seed. Note that if rand is called without a seed "
        "the previous state of the random number generator is advanced for the "
        "currently processing element. This state is determined by the last call to "
        "rand() with a given seed. If rand is not called with a seed, the generator "
        "advances continuously across different elements which can produce non-"
        "deterministic results. It is important that rand is always called with a "
        "seed at least once for deterministic results.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Rand()); }

    Rand() : FunctionBase({
        FunctionSignature<double()>::create
            ((double(*)())(Rand::rand), std::string("rand")),
        FunctionSignature<double(double)>::create
            ((double(*)(double))(Rand::rand), std::string("randd")),
        FunctionSignature<double(int32_t)>::create
            ((double(*)(int32_t))(Rand::rand), std::string("randi"))
    }) {}

private:

    static double rand(const uint32_t* seed)
    {
        using ThreadLocalEngineContainer =
            tbb::enumerable_thread_specific<boost::mt19937>;

        // Obtain thread-local engine (or create if it doesn't exist already).
        static ThreadLocalEngineContainer ThreadLocalEngines;
        static boost::uniform_01<double> Generator;

        boost::mt19937& engine = ThreadLocalEngines.local();
        if (seed) {
            engine.seed(static_cast<boost::mt19937::result_type>(*seed));
        }

        // Once we have seeded the random number generator, we then evaluate it,
        // which returns a floating point number in the range [0,1)
        return Generator(engine);
    }

    static double rand() { return Rand::rand(nullptr); }

    static double rand(double seed)
    {
        // We initially hash the double-precision seed with `boost::hash`. The
        // important thing about the hash is that it produces a "reliable" hash value,
        // taking into account a number of special cases for floating point numbers
        // (e.g. -0 and +0 must return the same hash value, etc). Other than these
        // special cases, this function will usually just copy the binary
        // representation of a float into the resultant `size_t`
        const size_t hash = boost::hash<double>()(seed);

        // Now that we have a reliable hash (with special floating-point cases taken
        // care of), we proceed to use this hash to seed a random number generator.
        // The generator takes an unsigned int, which is not guaranteed to be the
        // same size as size_t.
        //
        // So, we must convert it. I should note that the OpenVDB math libraries will
        // do this for us, but its implementation static_casts `size_t` to `unsigned int`,
        // and because `boost::hash` returns a binary copy of the original
        // double-precision number in almost all cases, this ends up producing noticable
        // patterns in the result (e.g. by truncating the upper 4 bytes, values of 1.0,
        // 2.0, 3.0, and 4.0 all return the same hash value because their lower 4 bytes
        // are all zero).
        //
        // We use the `hashToSeed` function to reduce our `size_t` to an `unsigned int`,
        // whilst taking all bits in the `size_t` into account.
        const uint32_t uintseed = hashToSeed<uint32_t>(hash);
        return Rand::rand(&uintseed);
    }

    static double rand(int32_t seed)
    {
        const uint32_t uintseed = static_cast<uint32_t>(seed);
        return Rand::rand(&uintseed);
    }
};

struct SimplexNoise : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("simplexnoise",
        "Compute simplex noise at coordinates x, y and z. Coordinates which are "
        "not provided will be set to 0.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new SimplexNoise()); }

    SimplexNoise() : FunctionBase({
        FunctionSignature<double(double)>::create
            ((double(*)(double))(noise), std::string("simplexnoised")),
        FunctionSignature<double(double,double)>::create
            ((double(*)(double,double))(noise), std::string("simplexnoisedd")),
        FunctionSignature<double(double,double,double)>::create
            ((double(*)(double,double,double))(noise), std::string("simplexnoiseddd")),
        FunctionSignature<double(const double(*)[3])>::create
            ((double(*)(const double(*)[3]))(noise), std::string("simplexnoisev3d"))

    }) {}

    // Open simplex noise - Visually axis-decorrelated coherent noise algorithm
    // based on the simplectic honeycomb.
    // See https://gist.github.com/KdotJPG/b1270127455a94ac5d19

    static double noise(double x, double y, double z)
    {
        static const OSN::OSNoise noiseGenerator = OSN::OSNoise();
        const double result = noiseGenerator.eval<double>(x, y, z);

        // adjust result so that it lies between 0 and 1, since Noise::eval returns
        // a number between -1 and 1
        return (result + 1.0) * 0.5;
    }

private:
    static double noise(double x, double y) { return noise(x, y, 0.0); }
    static double noise(double x) { return noise(x, 0.0, 0.0); }
    static double noise(const double (*in)[3]) { return noise((*in)[0], (*in)[1], (*in)[2]); }
};

struct CurlSimplexNoise : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_curlsimplexnoise",
            "Internal function for computing curl noise.")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
                DECLARE_FUNCTION_SIGNATURE_OUTPUT(curlnoise_vec3d<SimplexNoise>),
                DECLARE_FUNCTION_SIGNATURE_OUTPUT(curlnoise_ddd<SimplexNoise>)
            }) {}
    };

    DEFINE_IDENTIFIER_DOC("curlsimplexnoise",
        "Generates divergence-free 3D noise, computed using a curl function on Simplex Noise.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new CurlSimplexNoise()); }

    CurlSimplexNoise() : FunctionBase({
        FunctionSignature<V3D*(V3D*)>::create
            (nullptr, std::string("curlsimplexnoisevd"), 0),
        FunctionSignature<V3D*(double,double,double)>::create
            (nullptr, std::string("curlsimplexnoisesd"), 0)
    }) {}

    inline void
    getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_curlsimplexnoise");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final {
        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(args, globals, B, &results);
        assert(!results.empty());
        return results.front();
    }
};

struct Print : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("print", "Prints the input to the standard output "
        "stream. Warning: This will be run for every element.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Print()); }

    Print() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(print<char*>),
        DECLARE_FUNCTION_SIGNATURE(print<double>),
        DECLARE_FUNCTION_SIGNATURE(print<float>),
        DECLARE_FUNCTION_SIGNATURE(print<int32_t>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec2<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec2<float>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec2<double>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec3<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec3<float>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec3<double>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec4<int32_t>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec4<float>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Vec4<double>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Mat3<double>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(printarray<openvdb::math::Mat4<double>>),
    }) {}

private:
    template <typename T>
    inline static void print(const T v) {
        std::cout << v << std::endl;
    }
    template <typename T>
    inline static void printarray(const T* v) {
        std::cout << *v << std::endl;
    }
};

struct Hash : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("hash", "Return a hash of the provided string.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Hash()); }

    Hash() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(hash),
    }) {}

private:
    inline static int64_t hash(const char* str) {
        return static_cast<int64_t>(std::hash<std::string>{}(std::string(str)));
    }
};

struct Atoi : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("atoi", "Parses the string input interpreting its "
        "content as an integral number, which is returned as a value of type int.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atoi()); }

    OPENVDB_NO_ATTRIBUTES_WARNING_BEGIN
    Atoi() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(std::atoi),
        DECLARE_FUNCTION_SIGNATURE(std::atol)
    }) {}
    OPENVDB_NO_ATTRIBUTES_WARNING_END
};

struct Atof : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("atof", "Parses the string input, interpreting its "
        "content as a floating point number and returns its value as a double.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new Atof()); }

    OPENVDB_NO_ATTRIBUTES_WARNING_BEGIN
    Atof() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(std::atof)
    }) {}
    OPENVDB_NO_ATTRIBUTES_WARNING_END
};

struct Signbit : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("signbit", "Determines if the given floating point "
        "number input is negative.")

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
    DEFINE_IDENTIFIER_DOC("abs", "Computes the absolute value of an integer number.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Abs(PrioritiseIRGeneration()) : new Abs());
    }

    Abs() : FunctionBase({
        FunctionSignature<int32_t(int32_t)>::create
            ((int32_t(*)(int32_t))(std::abs), std::string("absi")),
        FunctionSignature<long(long)>::create
            ((long(*)(long))(std::abs), std::string("absl"))
    }) {}

    Abs(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<int32_t(int32_t)>::create
            (nullptr, std::string("absi"), 0)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        // arithmetic shift right
        llvm::Value* mask =
            B.CreateAShr(args[0], LLVMType<int32_t>::get(B.getContext(), 31));
        llvm::Value* xorResult = binaryOperator(args[0], mask, ast::tokens::BITXOR, B);
        return binaryOperator(xorResult, mask, ast::tokens::MINUS, B);
    }
};

struct Lerp : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("lerp",
        "Performs bilinear interpolation between the values. If the amount is "
        "outside the range 0 to 1, the values will be extrapolated linearly. If "
        "amount is 0, the first value is returned. If it is 1, the second value "
        "is returned.")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new Lerp());
    }

    Lerp() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(lerp<float>),
        DECLARE_FUNCTION_SIGNATURE(lerp<double>)
    }) {}

private:
    template <typename ValueT>
    static ValueT lerp(ValueT a, ValueT b, ValueT x) {
        return (ValueT(1.0) - x) * a + x * b;
    }
};

struct External
{
    template <typename T>
    inline static T find(const char* const name, const void* const data)
    {
        const ax::CustomData* const customData =
            static_cast<const ax::CustomData* const>(data);
        const std::string nameStr(name);

        const TypedMetadata<T>* const metaData =
            customData->getData<TypedMetadata<T>>(nameStr);
        return metaData ? metaData->value() : zeroVal<T>();
    }

    template <typename T>
    inline static void findv3f(const char* const name,
        const void* const data, T (*out)[3])
    {
        const openvdb::math::Vec3<T> result = find<openvdb::math::Vec3<T>>(name, data);
        for (size_t i = 0; i < 3; ++i) {
            (*out)[i] = result[static_cast<int>(i)];
        }
    }
};

struct ExternalScalar : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_findf",
            "Internal function for looking up a custom float value.")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE(External::find<float>)
        }) {}
    };

    DEFINE_IDENTIFIER_DOC("external", "Find a custom user parameter with a given "
        "name of type 'float' in the Custom data provided to the AX compiler. If "
        "the data can not be found, or is not of the expected type 0.0f is returned.")

    inline static FunctionBase::Ptr create(const FunctionOptions&) {
        return Ptr(new ExternalScalar());
    }

    ExternalScalar() : FunctionBase({
        FunctionSignature<float(const char* const)>::create
            (nullptr, std::string("externalf"), 0)
    }) {}

    inline void
    getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_findf");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override
    {
        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(globals.at("custom_data"));
        Internal func;
        return func.execute(internalArgs, globals, B);
     }
};

struct ExternalVector : public FunctionBase
{
    struct Internal : public FunctionBase {
        DEFINE_IDENTIFIER_DOC("internal_findv3f",
            "Internal function for looking up a custom vector float value.")
        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(External::findv3f<float>)
        }) {}
    };

    DEFINE_IDENTIFIER_DOC("externalv", "Find a custom user parameter with a given "
        "name of type 'vector float' in the Custom data provided to the AX compiler. "
        "If the data can not be found, or is not of the expected type { 0.0f, 0.0f, "
        "0.0f } is returned.")

    inline static FunctionBase::Ptr create(const FunctionOptions&) {
        return Ptr(new ExternalVector());
    }

    ExternalVector() : FunctionBase({
        FunctionSignature<V3F*(const char* const)>::create
            (nullptr, std::string("externalv3f"), 0)
    }) {}

    inline void
    getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_findv3f");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override
    {
        std::vector<llvm::Value*> internalArgs(args);
        internalArgs.emplace_back(globals.at("custom_data"));
        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(internalArgs, globals, B, &results);
        return results.front();
     }
};

struct Min : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("min", "Returns the smaller of the given values.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Min(PrioritiseIRGeneration()) : new Min());
    }

    Min() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(Min::min<double>),
        DECLARE_FUNCTION_SIGNATURE(Min::min<float>),
        DECLARE_FUNCTION_SIGNATURE(Min::min<int32_t>)
    }) {}

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
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        llvm::Value* result =
            binaryOperator(args[0], args[1], ast::tokens::MORETHAN, B);
        return B.CreateSelect(result, args[1], args[0], "mintmp");
    }

private:
    template<typename T>
    inline static T min(T a, T b) { return std::min(a,b); }
};

struct Max : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("max", "Returns the larger of the given values.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Max(PrioritiseIRGeneration()) : new Max());
    }

    Max() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(Max::max<double>),
        DECLARE_FUNCTION_SIGNATURE(Max::max<float>),
        DECLARE_FUNCTION_SIGNATURE(Max::max<int32_t>)
    }) {}

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
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        llvm::Value* result =
            binaryOperator(args[0], args[1], ast::tokens::MORETHAN, B);
        return B.CreateSelect(result, args[0], args[1], "maxtmp");
    }

private:
    template<typename T>
    inline static T max(T a, T b) { return std::max(a,b); }
};

struct Tan : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("tan",
        "Computes the tangent of arg (measured in radians).")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Tan(PrioritiseIRGeneration()) : new Tan());
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
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        llvm::Module* M = B.GetInsertBlock()->getParent()->getParent();
        llvm::Type* type = args[0]->getType();
        llvm::Function* sinFunction =
            llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::sin, type);
        llvm::Function* cosFunction =
            llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::cos, type);

        llvm::Value* sin = B.CreateCall(sinFunction, args[0], "sintmp");
        llvm::Value* cos = B.CreateCall(cosFunction, args[0], "costmp");
        return binaryOperator(sin, cos, ast::tokens::DIVIDE, B);
    }
};

struct LengthSq : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("lengthsq",
        "Returns the squared length of the given vector")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new LengthSq(PrioritiseIRGeneration()) : new LengthSq());
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
               llvm::IRBuilder<>& B)
    {
        arrayUnpack(ptrToVector, elements, B, /*load*/true);
        assert(elements.size() == 3);

        llvm::Value* v1 = binaryOperator(elements[0], elements[0], ast::tokens::MULTIPLY, B);
        llvm::Value* v2 = binaryOperator(elements[1], elements[1], ast::tokens::MULTIPLY, B);
        llvm::Value* v3 = binaryOperator(elements[2], elements[2], ast::tokens::MULTIPLY, B);
        llvm::Value* result = binaryOperator(v1, v2, ast::tokens::PLUS, B);
        return binaryOperator(result, v3, ast::tokens::PLUS, B);
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> elements;
        return doGenerate(args[0], elements, B);
    }

private:
    template <typename T>
    inline static T lensq(T (*in)[3]) {
        return openvdb::math::Vec3<T>(in[0]).lengthSqr();
    }
};

struct Length : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("length", "Returns the length of the given vector")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Length(PrioritiseIRGeneration()) : new Length());
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
           llvm::IRBuilder<>& B)
    {
        llvm::Value* result = LengthSq::doGenerate(ptrToVector, elements, B);

        llvm::Type* floatT = LLVMType<float>::get(B.getContext());
        if (result->getType()->isIntegerTy()) {
           result = arithmeticConversion(result, floatT, B);
        }

        llvm::Module* M = B.GetInsertBlock()->getParent()->getParent();
        llvm::Function* sqrt =
            llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::sqrt, result->getType());
        return B.CreateCall(sqrt, result, "lengthsqrt");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> elements;
        return doGenerate(args[0], elements, B);
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
        DEFINE_IDENTIFIER_DOC("internal_normalize",
            "Internal function for performing vector normalization")

        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(normalize<double>),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(normalize<float>)
        }) {}

        private:
            template <typename T>
            inline static void normalize(T (*in)[3], T (*out)[3]) {
                openvdb::math::Vec3<T> inv(in[0]);
                // no tolerance, match IR impl
                inv.normalize(T(0.0));
                for (size_t i = 0; i < 3; ++i) {
                    out[0][i] = inv[static_cast<int>(i)];
                }
            }
    };

    DEFINE_IDENTIFIER_DOC("normalize",
        "Returns the normalized result of the given vector.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Normalize(PrioritiseIRGeneration()) : new Normalize());
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

    inline void
    getDependencies(std::vector<std::string>& identifiers) const override {
        if (mExternalCall) identifiers.emplace_back("internal_normalize");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        llvm::Value* result = nullptr;

        if (mExternalCall) {
            std::vector<llvm::Value*> results;
            Internal func;
            func.execute(args, globals, B, &results);
            assert(!results.empty());
            result = results.front();
        }
        else {
            std::vector<llvm::Value*> elements;
            llvm::Value* lSq = Length::doGenerate(args.front(), elements, B);

            if (elements[0]->getType()->isIntegerTy()) {
               llvm::Type* floatT = LLVMType<float>::get(B.getContext());
               elements[0] = arithmeticConversion(elements[0], floatT, B);
               elements[1] = arithmeticConversion(elements[1], floatT, B);
               elements[2] = arithmeticConversion(elements[2], floatT, B);
            }

            // the following is always done at fp precision

            llvm::Value* one = llvm::ConstantFP::get(lSq->getType(), 1.0);
            llvm::Value* oneDividedByLength = B.CreateFDiv(one, lSq);

            elements[0] = B.CreateFMul(elements[0], oneDividedByLength);
            elements[1] = B.CreateFMul(elements[1], oneDividedByLength);
            elements[2] = B.CreateFMul(elements[2], oneDividedByLength);

            // pack and store in second argument
            result = arrayPack(elements, B);
        }

        return result;
    }

private:
    const bool mExternalCall;
};

struct DotProd : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("dot", "Computes the dot product of two vectors")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new DotProd(PrioritiseIRGeneration()) : new DotProd());
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
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> v1, v2;
        arrayUnpack(args[0], v1, B, /*load*/true);
        arrayUnpack(args[1], v2, B, /*load*/true);

        v1[0] = binaryOperator(v1[0], v2[0], ast::tokens::MULTIPLY, B);
        v1[1] = binaryOperator(v1[1], v2[1], ast::tokens::MULTIPLY, B);
        v1[2] = binaryOperator(v1[2], v2[2], ast::tokens::MULTIPLY, B);

        llvm::Value* result = binaryOperator(v1[0], v1[1], ast::tokens::PLUS, B);
        result = binaryOperator(result, v1[2], ast::tokens::PLUS, B);
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
        DEFINE_IDENTIFIER_DOC("internal_cross",
            "Internal function for performing vector cross products")

        inline static Ptr create(const FunctionOptions&) { return Ptr(new Internal()); }
        Internal() : FunctionBase({
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<double>),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<float>),
            DECLARE_FUNCTION_SIGNATURE_OUTPUT(cross<int32_t>)
        }) {}

    private:
        template <typename T>
        inline static void cross(T (*in1)[3], T (*in2)[3], T (*out)[3]) {
            const openvdb::math::Vec3<T> inv1(*in1);
            const openvdb::math::Vec3<T> inv2(*in2);
            const openvdb::math::Vec3<T> result = inv1.cross(inv2);
            for (size_t i = 0; i < 3; ++i) {
                out[0][i] = result[static_cast<int>(i)];
            }
        }
    };

    DEFINE_IDENTIFIER_DOC("cross", "Computes the cross product of two vectors")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new CrossProd()); }

    CrossProd() : FunctionBase({
        FunctionSignature<V3D*(V3D*,V3D*)>::create
            (nullptr, std::string("crossprodd"), 0),
        FunctionSignature<V3F*(V3F*,V3F*)>::create
            (nullptr, std::string("crossprodf"), 0),
        FunctionSignature<V3I*(V3I*,V3I*)>::create
            (nullptr, std::string("crossprodi"), 0)
    }) {}

    inline void
    getDependencies(std::vector<std::string>& identifiers) const override {
        identifiers.emplace_back("internal_cross");
    }

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> results;
        Internal func;
        func.execute(args, globals, B, &results);
        assert(!results.empty());
        return results.front();
    }
};

struct Clamp : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("clamp", "Clamps the first argument to the minimum "
        "second argument value and maximum third argument value")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new Clamp(PrioritiseIRGeneration()) : new Clamp());
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
         llvm::IRBuilder<>& B) const override
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

        llvm::LLVMContext& context = B.getContext();
        llvm::Module* M = B.GetInsertBlock()->getParent()->getParent();

        llvm::Type* type = args[0]->getType();
        const bool isInt = type->isIntegerTy();

        llvm::Value* temp = binaryOperator(args[0], args[2], ast::tokens::MINUS, B);

        // We have to use the IR implementation of ABS as ABS is not currently
        // registered if not used explicitly! @TODO!
        Abs absFunc = Abs(PrioritiseIRGeneration());

        if (isInt) temp = absFunc.execute({temp}, globals, B);
        else {
            llvm::Function* abs =
                llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::fabs, type);
            temp = B.CreateCall(abs, temp, "abs");
        }

        temp = binaryOperator(args[2], temp, ast::tokens::MINUS, B);
        temp = binaryOperator(args[0], temp, ast::tokens::PLUS, B);

        llvm::Value* two = isInt ?
            LLVMType<int32_t>::get(context, 2) : llvm::ConstantFP::get(type, 2.0);

        llvm::Value* startMultTwo =
            binaryOperator(two, args[1], ast::tokens::MULTIPLY, B);
        llvm::Value* sub = binaryOperator(temp, startMultTwo, ast::tokens::MINUS, B);

        llvm::Value* result = nullptr;
        if (isInt) result = absFunc.execute({sub}, globals, B);
        else {
            llvm::Function* abs =
                llvm::Intrinsic::getDeclaration(M, llvm::Intrinsic::fabs, type);
            result = B.CreateCall(abs, sub, "abs");
        }

        result = binaryOperator(startMultTwo, result, ast::tokens::PLUS, B);
        result = binaryOperator(temp, result, ast::tokens::PLUS, B);

        llvm::Type* doubleT = LLVMType<double>::get(context);
        result = arithmeticConversion(result, doubleT, B);

        llvm::Value* pointTwoFive = LLVMType<double>::get(context, 0.25);
        return binaryOperator(result, pointTwoFive, ast::tokens::MULTIPLY, B);
    }
};

struct Fit : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("fit", "Fit the first argument to the output range by "
        "first clamping the value between the second and third input range "
        "arguments and then remapping the result to the output range fourth and "
        "fifth arguments")

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
         llvm::IRBuilder<>& B) const override final
    {
        /*
                (outMax - outMin)(x - inMin)
        f(x) = ----------------------------  + outMin
                       inMax - inMin

        if inMax == inMin, f(x) = (outMax + outMin) / 2.0

        */

        // NOTE: this also performs a clamp on the ordered input range
        // @TODO revisit. If this is the best thing to do, should add conditional
        // branching so that the clamping math is never executed when the value
        // is inside

        // args[0] = x
        // args[1] = inMin
        // args[2] = inMax
        // args[3] = outMin
        // args[4] = outMax

        std::vector<llvm::Value*> argcopy(args);

        // select the precision at which to perform

        llvm::Type* precision = argcopy[0]->getType();
        if (precision->isIntegerTy()) {
            precision = LLVMType<double>::get(B.getContext());
        }

        // See if the input range has a valid magnitude .i.e. the values are not the same

        llvm::Value* isInputRangeValid =
            binaryOperator(argcopy[1], argcopy[2], ast::tokens::NOTEQUALS, B);

        // clamp the input to the ORDERED inMin to inMax range

        llvm::Value* minRangeComp =
            binaryOperator(argcopy[1], argcopy[2], ast::tokens::LESSTHAN, B);
        llvm::Value* minInputRange = B.CreateSelect(minRangeComp, argcopy[1], argcopy[2]);
        llvm::Value* maxInputRange = B.CreateSelect(minRangeComp, argcopy[2], argcopy[1]);

        // clamp
        {
            // We have to use the IR implementation of Clamp as Clamp is not currently registered
            // if not used explicitly! @TODO!
            Clamp clampOp = Clamp(PrioritiseIRGeneration());
            argcopy[0] = clampOp.execute({ argcopy[0], minInputRange, maxInputRange }, globals, B);
        }

        // cast all (the following requires floating point precision)

        for (auto& arg : argcopy) arg = arithmeticConversion(arg, precision, B);

        llvm::Value* valueMinusMin = B.CreateFSub(argcopy[0], argcopy[1]);
        llvm::Value* inputRange = B.CreateFSub(argcopy[2], argcopy[1]);
        llvm::Value* outputRange = B.CreateFSub(argcopy[4], argcopy[3]);

        llvm::Value* result = B.CreateFMul(outputRange, valueMinusMin);
        result = B.CreateFDiv(result, inputRange);  // NOTE - This can cause division by zero
        result = B.CreateFAdd(argcopy[3], result);

        // calculate the output range over 2 and use this value if the input range is invalid

        llvm::Value* outputRangeOverTwo = B.CreateFAdd(argcopy[3], argcopy[4]);
        llvm::Value* two = llvm::ConstantFP::get(precision, 2.0);
        outputRangeOverTwo = B.CreateFDiv(outputRangeOverTwo, two);

        return B.CreateSelect(isInputRangeValid, result, outputRangeOverTwo);
    }
};

struct MatIdentity3 : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("identity3", "Returns the 3x3 identity matrix")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatIdentity3(PrioritiseIRGeneration()));
    }

     MatIdentity3(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<M3F*()>::create
            (nullptr, std::string("identity3f"))
     }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>&,
        const std::unordered_map<std::string, llvm::Value*>&,
        llvm::IRBuilder<>& B) const override final
    {
        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(LLVMType<float>::get(B.getContext()), 9));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);
        llvm::Value* zero = LLVMType<float>::get(B.getContext(), 0.0f);
        llvm::Value* one = LLVMType<float>::get(B.getContext(), 1.0f);
        for (size_t i = 0; i < 9; ++i) {
            llvm::Value* m = ((i == 0 || i == 4 || i == 8) ? one : zero);
            B.CreateStore(m, elements[i]);
        }
        return resultPtr;
    }
};

struct MatIdentity4 : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("identity4", "Returns the 4x4 identity matrix")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatIdentity4(PrioritiseIRGeneration()));
    }

     MatIdentity4(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<M4F*()>::create
            (nullptr, std::string("identity4f"))
     }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>&,
        const std::unordered_map<std::string, llvm::Value*>&,
        llvm::IRBuilder<>& B) const override final
    {
        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(LLVMType<float>::get(B.getContext()), 16));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);
        llvm::Value* zero = LLVMType<float>::get(B.getContext(), 0.0f);
        llvm::Value* one = LLVMType<float>::get(B.getContext(), 1.0f);
        for (size_t i = 0; i < 16; ++i) {
            llvm::Value* m = ((i == 0 || i == 5 || i == 10 || i == 15) ? one : zero);
            B.CreateStore(m, elements[i]);
        }
        return resultPtr;
    }
};

struct MatDeterminant : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("determinant", "Returns the determinant of a matrix.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new MatDeterminant(PrioritiseIRGeneration()) : new MatDeterminant());
    }

    MatDeterminant(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<float(M3F*)>::create
            (nullptr, std::string("m3fdeterminant")),
        FunctionSignature<double(M3D*)>::create
            (nullptr, std::string("m3ddeterminant")),
        FunctionSignature<float(M4F*)>::create
            (nullptr, std::string("m4fdeterminant")),
        FunctionSignature<double(M4D*)>::create
            (nullptr, std::string("m4ddeterminant"))
    }) {}

    MatDeterminant() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(determinant<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE(determinant<openvdb::math::Mat3<double>>),
        DECLARE_FUNCTION_SIGNATURE(determinant<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(determinant<openvdb::math::Mat4<double>>)
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>& globals,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1;
        arrayUnpack(args[0], m1, B, /*load*/true);
        const size_t size = m1.size();
        assert(size == 9 || size == 16);

        if (size == 9) {
            // 3 by 3 determinant
            llvm::Value* e1 = binaryOperator(m1[4], m1[8], ast::tokens::MULTIPLY, B);
            llvm::Value* e2 = binaryOperator(m1[5], m1[7], ast::tokens::MULTIPLY, B);
            llvm::Value* c0 = binaryOperator(e1, e2, ast::tokens::MINUS, B);

            e1 = binaryOperator(m1[5], m1[6], ast::tokens::MULTIPLY, B);
            e2 = binaryOperator(m1[3], m1[8], ast::tokens::MULTIPLY, B);
            llvm::Value* c1 = binaryOperator(e1, e2, ast::tokens::MINUS, B);

            e1 = binaryOperator(m1[3], m1[7], ast::tokens::MULTIPLY, B);
            e2 = binaryOperator(m1[4], m1[6], ast::tokens::MULTIPLY, B);
            llvm::Value* c2 = binaryOperator(e1, e2, ast::tokens::MINUS, B);

            c0 = binaryOperator(m1[0], c0, ast::tokens::MULTIPLY, B);
            c1 = binaryOperator(m1[1], c1, ast::tokens::MULTIPLY, B);
            c2 = binaryOperator(m1[2], c2, ast::tokens::MULTIPLY, B);

            c0 = binaryOperator(c0, c1, ast::tokens::PLUS, B);
            c0 = binaryOperator(c0, c2, ast::tokens::PLUS, B);
            return c0;
        }
        else {
            // 4 by 4 determinant
            assert(m1.size() == 16);
            // @todo  Avoid allocating here? Signature current expects a valid
            //        variable. Probably will be optimised out anyway
            llvm::Value* subMat = B.CreateAlloca(llvm::ArrayType::get(m1.front()->getType(), 9));
            std::vector<llvm::Value*> elements;
            arrayUnpack(subMat, elements, B, /*load elements*/false);

            llvm::Value* result = llvm::ConstantFP::get(m1.front()->getType(), 0.0);
            for (size_t i = 0; i < 4; ++i) {
                size_t sourceIndex = 0, targetIndex = 0;
                for (size_t j = 0; j < 4; ++j) {
                    for (size_t k = 0; k < 4; ++k) {
                        if ((k != i) && (j != 0)) {
                            B.CreateStore(m1[sourceIndex], elements[targetIndex]);
                            ++targetIndex;
                        }
                        ++sourceIndex;
                    }
                }
                llvm::Value* subResult = this->generate({subMat}, globals, B);
                subResult = binaryOperator(m1[i], subResult, ast::tokens::MULTIPLY, B);

                if (i % 2) result = binaryOperator(result, subResult, ast::tokens::MINUS, B);
                else       result = binaryOperator(result, subResult, ast::tokens::PLUS, B);
            }

            return result;
        }
    }

private:
    template <typename MatrixT>
    inline static typename MatrixT::ValueType
    determinant(const MatrixT* const matrix) {
        return matrix->det();
    }
};

struct MatTranspose : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("transpose", "Transpose of a matrix")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatTranspose(PrioritiseIRGeneration()));
    }

     MatTranspose(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<M3D*(M3D*)>::create
            (nullptr, std::string("m3dtranspose")),
        FunctionSignature<M3F*(M3F*)>::create
            (nullptr, std::string("m3ftranspose")),
        FunctionSignature<M4D*(M4D*)>::create
            (nullptr, std::string("m4dtranspose")),
        FunctionSignature<M4F*(M4F*)>::create
            (nullptr, std::string("m4ftranspose"))
     }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
        const std::unordered_map<std::string, llvm::Value*>&,
        llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1;
        arrayUnpack(args[0], m1, B, /*load*/true);
        assert(m1.size() == 9 || m1.size() == 16);
        const size_t dim = m1.size() == 9 ? 3 : 4;

        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(m1.front()->getType(), m1.size()));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);

        for (size_t i = 0; i < dim; ++i) {
            for (size_t j = 0; j < dim; ++j) {
                const size_t source = (i*dim) + j;
                const size_t target = (j*dim) + i;
                B.CreateStore(m1[source], elements[target]);
            }
        }

        return resultPtr;
    }
};

struct MatPrescale : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("prescale",
        "Pre-scale a given matrix by the provided vector.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new MatPrescale(PrioritiseIRGeneration()) : new MatPrescale());
    }

    MatPrescale() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(prescale<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(prescale<openvdb::math::Mat4<double>>)
    }) {}

    MatPrescale(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<void(M4F*, V3F*)>::create
            (nullptr, std::string("m4fprescale")),
        FunctionSignature<void(M4D*, V3D*)>::create
            (nullptr, std::string("m4dprescale"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1, v1;
        arrayUnpack(args[0], m1, B, /*load*/false);
        arrayUnpack(args[1], v1, B, /*load*/true);
        assert(m1.size() == 16);
        assert(v1.size() == 3);

        // modify first 3 mat rows, all columns
        for (size_t row = 0; row < 3; ++row) {
            for (size_t col = 0; col < 4; ++col) {
                const size_t idx = (row*4) + col;
                assert(idx <= 11);
                llvm::Value* m1v = B.CreateLoad(m1[idx]);
                m1v = binaryOperator(m1v, v1[row], ast::tokens::MULTIPLY, B);
                B.CreateStore(m1v, m1[idx]);
            }
        }

        return nullptr;
    }

private:
    template <typename MatrixT>
    inline static void prescale(MatrixT* const matrix, const double (*sc)[3]) {
        matrix->preScale(openvdb::Vec3d(sc[0]));
    }
};

struct MatPostscale : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("postscale",
        "Post-scale a given matrix by the provided vector.")

    inline static Ptr create(const FunctionOptions& op) {
        return Ptr(op.mPrioritiseFunctionIR ?
            new MatPostscale(PrioritiseIRGeneration()) : new MatPostscale());
    }

    MatPostscale() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(postscale<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(postscale<openvdb::math::Mat4<double>>)
    }) {}

    MatPostscale(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<void(M4F*, V3F*)>::create
            (nullptr, std::string("m4fpostscale")),
        FunctionSignature<void(M4D*, V3D*)>::create
            (nullptr, std::string("m4dpostscale"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1, v1;
        arrayUnpack(args[0], m1, B, /*load*/false);
        arrayUnpack(args[1], v1, B, /*load*/true);
        assert(m1.size() == 16);
        assert(v1.size() == 3);

        // modify first 3 elements in all mat rows
        for (size_t row = 0; row < 4; ++row) {
            for (size_t col = 0; col < 3; ++col) {
                const size_t idx = (row*4) + col;
                assert(idx <= 14);
                llvm::Value* m1v = B.CreateLoad(m1[idx]);
                m1v = binaryOperator(m1v, v1[row], ast::tokens::MULTIPLY, B);
                B.CreateStore(m1v, m1[idx]);
            }
        }

        return nullptr;
    }

private:
    template <typename MatrixT>
    inline static void postscale(MatrixT* const matrix, const double (*sc)[3]) {
        matrix->postScale(openvdb::Vec3d(sc[0]));
    }
};

struct MatMatMult : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("mmmult", "Mat*Mat")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatMatMult(PrioritiseIRGeneration()));
    }

    MatMatMult(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<M3D*(M3D*, M3D*)>::create
            (nullptr, std::string("m3dmult")),
        FunctionSignature<M3F*(M3F*, M3F*)>::create
            (nullptr, std::string("m3fmult")),
        FunctionSignature<M4D*(M4D*, M4D*)>::create
            (nullptr, std::string("m4dmult")),
        FunctionSignature<M4F*(M4F*, M4F*)>::create
            (nullptr, std::string("m4fmult"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1, m2;
        arrayUnpack(args[0], m1, B, /*load*/true);
        arrayUnpack(args[1], m2, B, /*load*/true);

        assert(m1.size() == 9 || m1.size() == 16);
        const size_t dim = m1.size() == 9 ? 3 : 4;

        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(m1.front()->getType(), m1.size()));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);

        llvm::Value* e3 = nullptr, *e4 = nullptr;
        for (size_t i = 0; i < dim; ++i) {
            const size_t row = i*dim;
            for (size_t j = 0; j < dim; ++j) {
                llvm::Value* e1 = binaryOperator(m1[0+row], m2[j], ast::tokens::MULTIPLY, B);
                llvm::Value* e2 = binaryOperator(m1[1+row], m2[dim+j], ast::tokens::MULTIPLY, B);
                if (dim >=3) e3 = binaryOperator(m1[2+row], m2[(dim*2)+j], ast::tokens::MULTIPLY, B);
                if (dim >=4) e4 = binaryOperator(m1[3+row], m2[(dim*3)+j], ast::tokens::MULTIPLY, B);
                e1 = binaryOperator(e1, e2, ast::tokens::PLUS, B);
                if (dim >=3) e1 = binaryOperator(e1, e3, ast::tokens::PLUS, B);
                if (dim >=4) e1 = binaryOperator(e1, e4, ast::tokens::PLUS, B);
                B.CreateStore(e1, elements[row+j]);
            }
        }

        return resultPtr;
    }
};

struct MatTransform : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("transform", "Return the transformed vector by this "
        "matrix. This function is equivalent to post-multiplying the matrix.")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatTransform(PrioritiseIRGeneration()));
    }

    MatTransform(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<V3D*(V3D*, M3D*)>::create
            (nullptr, std::string("transformv3m3d")),
        FunctionSignature<V3F*(V3F*, M3F*)>::create
            (nullptr, std::string("transformv3m3f")),
        FunctionSignature<V3D*(V3D*,M4D*)>::create
            (nullptr, std::string("transformv3m4d")),
        FunctionSignature<V3F*(V3F*,M4F*)>::create
            (nullptr, std::string("transformv3m4f")),
        FunctionSignature<V4D*(V4D*, M4D*)>::create
            (nullptr, std::string("transformv4m4d")),
        FunctionSignature<V4F*(V4F*, M4F*)>::create
            (nullptr, std::string("transformv4m4f"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1, v1;
        arrayUnpack(args[0], v1, B, /*load*/true);
        arrayUnpack(args[1], m1, B, /*load*/true);

        const size_t vec = v1.size();
        const size_t dim = (m1.size() == 9 ? 3 : 4);

        assert(m1.size() == 9 || m1.size() == 16);
        assert(vec == 3 || vec == 4);

        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(v1.front()->getType(), vec));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);
        assert(elements.size() == vec);

        // vec * mat
        llvm::Value* e3 = nullptr, *e4 = nullptr;
        for (size_t i = 0; i < vec; ++i) {
            llvm::Value* e1 = binaryOperator(v1[0], m1[i+(0*dim)], ast::tokens::MULTIPLY, B);
            llvm::Value* e2 = binaryOperator(v1[1], m1[i+(1*dim)], ast::tokens::MULTIPLY, B);
            if (dim >= 3) e3 = binaryOperator(v1[2], m1[i+(2*dim)], ast::tokens::MULTIPLY, B);
            if (dim == 4) {
                if (vec == 3) e4 = m1[i+(3*dim)];
                else if (vec == 4) e4 = binaryOperator(v1[3], m1[i+(3*dim)], ast::tokens::MULTIPLY, B);
            }
            e1 = binaryOperator(e1, e2, ast::tokens::PLUS, B);
            if (e3) e1 = binaryOperator(e1, e3, ast::tokens::PLUS, B);
            if (e4) e1 = binaryOperator(e1, e4, ast::tokens::PLUS, B);
            B.CreateStore(e1, elements[i]);
        }

        return resultPtr;
    }
};

struct MatPretransform : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("pretransform", "Return the transformed vector by "
        "transpose of this matrix. This function is equivalent to "
        "pre-multiplying the matrix.")

    inline static Ptr create(const FunctionOptions&) {
        return Ptr(new MatPretransform(PrioritiseIRGeneration()));
    }

    MatPretransform(const PrioritiseIRGeneration&) : FunctionBase({
        FunctionSignature<V3D*(M3D*, V3D*)>::create
            (nullptr, std::string("pretransformm3v3d")),
        FunctionSignature<V3F*(M3F*, V3F*)>::create
            (nullptr, std::string("pretransformm3v3f")),
        FunctionSignature<V3D*(M4D*, V3D*)>::create
            (nullptr, std::string("pretransformm4v3d")),
        FunctionSignature<V3F*(M4F*, V3F*)>::create
            (nullptr, std::string("pretransformm4v3f")),
        FunctionSignature<V4D*(M4D*, V4D*)>::create
            (nullptr, std::string("pretransformm4v4d")),
        FunctionSignature<V4F*(M4F*, V4F*)>::create
            (nullptr, std::string("pretransformm4v4f"))
    }) {}

    llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
         const std::unordered_map<std::string, llvm::Value*>&,
         llvm::IRBuilder<>& B) const override final
    {
        std::vector<llvm::Value*> m1, v1;
        arrayUnpack(args[0], m1, B, /*load*/true);
        arrayUnpack(args[1], v1, B, /*load*/true);

        const size_t vec = v1.size();
        const size_t dim = (m1.size() == 9 ? 3 : 4);

        assert(dim == 9 || dim == 16);
        assert(vec == 3 || vec == 4);

        llvm::Value* resultPtr =
            B.CreateAlloca(llvm::ArrayType::get(v1.front()->getType(), vec));
        std::vector<llvm::Value*> elements;
        arrayUnpack(resultPtr, elements, B, /*load elements*/false);

        // mat * vec
        llvm::Value* e3 = nullptr, *e4 = nullptr;
        for (size_t i = 0; i < vec; ++i) {
            llvm::Value* e1 = binaryOperator(v1[0], m1[0+(i*dim)], ast::tokens::MULTIPLY, B);
            llvm::Value* e2 = binaryOperator(v1[1], m1[1+(i*dim)], ast::tokens::MULTIPLY, B);
            if (dim >= 3) e3 = binaryOperator(v1[2], m1[2+(i*dim)], ast::tokens::MULTIPLY, B);
            if (dim == 4) {
                if (vec == 3) e4 = m1[3+(i*dim)];
                else if (vec == 4) e4 = binaryOperator(v1[3], m1[3+(i*dim)], ast::tokens::MULTIPLY, B);
            }
            e1 = binaryOperator(e1, e2, ast::tokens::PLUS, B);
            if (e3) e1 = binaryOperator(e1, e3, ast::tokens::PLUS, B);
            if (e4) e1 = binaryOperator(e1, e4, ast::tokens::PLUS, B);
            B.CreateStore(e1, elements[i]);
        }

        return resultPtr;
    }
};

struct MatPolarDecompose : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("polardecompose",
        "Decompose an invertible 3x3 matrix into its orthogonal matrix and symmetric matrix components.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new MatPolarDecompose()); }

    MatPolarDecompose() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(polar_decompose<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE(polar_decompose<openvdb::math::Mat3<double>>)
    }) {}

private:
    template <typename MatrixT>
    inline static bool polar_decompose(const MatrixT* const input, MatrixT* ortho, MatrixT* symmetric) {
        return openvdb::math::polarDecomposition<MatrixT>(*input, *ortho, *symmetric);
    }
};

struct MatTrace : public FunctionBase
{
    DEFINE_IDENTIFIER_DOC("trace",
        "Return the trace of a matrix, the sum of the diagonal elements.")

    inline static Ptr create(const FunctionOptions&) { return Ptr(new MatTrace()); }

    MatTrace() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(trace<openvdb::math::Mat3<float>>),
        DECLARE_FUNCTION_SIGNATURE(trace<openvdb::math::Mat3<double>>),
        DECLARE_FUNCTION_SIGNATURE(trace<openvdb::math::Mat4<float>>),
        DECLARE_FUNCTION_SIGNATURE(trace<openvdb::math::Mat4<double>>)
    }) {}

private:
    template <typename MatrixT>
    inline static typename MatrixT::ValueType trace(const MatrixT* const input) {
        typename MatrixT::ValueType value(0.0);
        for (size_t i = 0; i < MatrixT::numRows(); ++i) {
           value += (*input)(static_cast<int>(i),static_cast<int>(i));
        }
        return value;
    }
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void insertStandardFunctions(FunctionRegistry& registry,
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

    // llvm instrinsics

    add("ceil", Ceil::create, false);
    add("cos", Cos::create, false);
    add("exp2", Exp2::create, false);
    add("exp", Exp::create, false);
    add("fabs", Fabs::create, false);
    add("floor", Floor::create, false);
    add("log10", Log10::create, false);
    add("log2", Log2::create, false);
    add("log", Log::create, false);
    add("pow", Pow::create, false);
    add("round", Round::create, false);
    add("sin", Sin::create, false);
    add("sqrt", Sqrt::create, false);

    // math

    add("abs", Abs::create, false);
    add("acos", Acos::create, false);
    add("asin", Asin::create, false);
    add("atan", Atan::create, false);
    add("atan2", Atan2::create, false);
    add("cbrt", Cbrt::create, false);
    add("clamp", Clamp::create, false);
    add("cosh", Cosh::create, false);
    add("curlsimplexnoise", CurlSimplexNoise::create, false);
    add("dot", DotProd::create, false);
    add("fit", Fit::create, false);
    add("lerp", Lerp::create, false);
    add("max", Max::create, false);
    add("min", Min::create, false);
    add("rand", Rand::create, false);
    add("signbit", Signbit::create, false);
    add("simplexnoise", SimplexNoise::create, false);
    add("sinh", Sinh::create, false);
    add("tan", Tan::create, false);
    add("tanh", Tanh::create, false);

    // string manip

    add("atof", Atof::create, false);
    add("atoi", Atoi::create, false);
    add("hash", Hash::create, false);
    add("print", Print::create, false);

    // math vector

    add("cross", CrossProd::create, false);
    add("length", Length::create, false);
    add("lengthsq", LengthSq::create, false);
    add("normalize", Normalize::create, false);

    // math matrix

    add("identity3", MatIdentity3::create, false);
    add("identity4", MatIdentity4::create, false);
    add("determinant", MatDeterminant::create, false);
    add("postscale", MatPostscale::create, false);
    add("prescale", MatPrescale::create, false);
    add("transpose", MatTranspose::create, false);
    add("transform", MatTransform::create, false);
    add("pretransform", MatPretransform::create, false);
    add("polardecompose", MatPolarDecompose::create, false);
    add("trace", MatTrace::create, false);
    add("mmmult", MatMatMult::create, true);

    // externals

    add("external", ExternalScalar::create, false);
    add("externalv", ExternalVector::create, false);

    // indirect internals

    add("internal_curlsimplexnoise", CurlSimplexNoise::Internal::create, true);
    add("internal_cross", CrossProd::Internal::create, true);
    add("internal_normalize", Normalize::Internal::create, true);
    add("internal_findf", ExternalScalar::Internal::create, true);
    add("internal_findv3f", ExternalVector::Internal::create, true);
}


}
}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
