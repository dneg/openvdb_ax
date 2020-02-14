///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2020 DNEG
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

#include "util.h"

#include <openvdb_ax/codegen/FunctionTypes.h>

#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include <string>

using Function = openvdb::ax::codegen::Function;
template <typename T>
using CFunction = openvdb::ax::codegen::CFunction<T>;
using Function = openvdb::ax::codegen::Function;
template <typename T>
using LLVMType = openvdb::ax::codegen::LLVMType<T>;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

struct TestFunctions
{
    static void SVFunction() {}
    static inline void SIVFunction() {}

    static void* SVpFunction() { return nullptr; }
    static inline void* SIVpFunction() { return nullptr; }

    static float SFFunction() { return float(); }
    static inline float SIFFunction() { return float(); }

    static float* SFpFunction() { return nullptr; }
    static inline float* SIFpFunction() { return nullptr; }

    static int16_t IFunctionScalar(bool,int16_t,int32_t,int64_t,float,double) { return short(); }
    static int32_t IFunctionScalarPtr(bool*,int16_t*,int32_t*,int64_t*,float*,double*) { return int(); }
    static int64_t IFunctionArray(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6]) { return long(); }
    static float FFunctionMix(bool,int16_t*,int32_t(*)[1],int64_t,float*,double(*)[2]) { return float(); }
    static double DFunctionMixc(const bool,int16_t* const,int32_t(*)[1],const int64_t,const float* const,const double(*)[2]) { return double(); }

    template <typename Type> static inline Type TemplateSelection0() { return Type(); }

    static void MultiPtrFunction(void*, void**, void***, float*, float**, float***) { }
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

class TestFunctionSignature : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestFunctionSignature);
    CPPUNIT_TEST(testCreateCBindings);
    CPPUNIT_TEST(testPrint);
    CPPUNIT_TEST(testTypeCreation);
    // @todo re-write this test
    //CPPUNIT_TEST(testReturnOutputs);
    CPPUNIT_TEST(testMatching);
    CPPUNIT_TEST(testCreateLLVMFunction);
    CPPUNIT_TEST_SUITE_END();

    void testCreateCBindings();
    void testPrint();
    void testTypeCreation();
    //void testReturnOutputs();
    void testMatching();
    void testCreateLLVMFunction();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFunctionSignature);

void
TestFunctionSignature::testCreateCBindings()
{
    CFunction<void()> test1("__void", nullptr);
    CPPUNIT_ASSERT_EQUAL(size_t(0), test1.size());
    CPPUNIT_ASSERT_EQUAL(uint64_t(0), test1.address());
    CPPUNIT_ASSERT_EQUAL(std::string("__void"), std::string(test1.symbol()));

    // test static void function

    CFunction<void()> SVFunction("SVFunction", &TestFunctions::SVFunction);
    CPPUNIT_ASSERT_EQUAL(size_t(0), SVFunction.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::SVFunction), SVFunction.address());
    CPPUNIT_ASSERT_EQUAL(std::string("SVFunction"), std::string(SVFunction.symbol()));

    // test inline static void function

    CFunction<void()> SIVFunction("SIVFunction", &TestFunctions::SIVFunction);
    CPPUNIT_ASSERT_EQUAL(size_t(0), SIVFunction.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::SIVFunction), SIVFunction.address());
    CPPUNIT_ASSERT_EQUAL(std::string("SIVFunction"), std::string(SIVFunction.symbol()));

    // test scalar arguments

    CFunction<int16_t(bool,int16_t,int32_t,int64_t,float,double)>
        scalars("IFunctionScalar", &TestFunctions::IFunctionScalar);
    CPPUNIT_ASSERT_EQUAL(size_t(6), scalars.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::IFunctionScalar), scalars.address());
    CPPUNIT_ASSERT_EQUAL(std::string("IFunctionScalar"), std::string(scalars.symbol()));

    // test scalar ptr arguments

    CFunction<int32_t(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
        scalarptrs("IFunctionScalarPtr", &TestFunctions::IFunctionScalarPtr);
    CPPUNIT_ASSERT_EQUAL(size_t(6), scalarptrs.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::IFunctionScalarPtr), scalarptrs.address());
    CPPUNIT_ASSERT_EQUAL(std::string("IFunctionScalarPtr"), std::string(scalarptrs.symbol()));

    // test array ptr arguments

    CFunction<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
        arrayptrs("IFunctionArray", &TestFunctions::IFunctionArray);
    CPPUNIT_ASSERT_EQUAL(size_t(6), arrayptrs.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::IFunctionArray), arrayptrs.address());
    CPPUNIT_ASSERT_EQUAL(std::string("IFunctionArray"), std::string(arrayptrs.symbol()));

    // test argument mixture

    CFunction<float(bool,int16_t*,int32_t(*)[1],int64_t,float*,double(*)[2])>
        mix("FFunctionMix", &TestFunctions::FFunctionMix);
    CPPUNIT_ASSERT_EQUAL(size_t(6), mix.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::FFunctionMix), mix.address());
    CPPUNIT_ASSERT_EQUAL(std::string("FFunctionMix"), std::string(mix.symbol()));

    // test argument mixture const

    CFunction<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
        constmix("DFunctionMixc", &TestFunctions::DFunctionMixc);
    CPPUNIT_ASSERT_EQUAL(size_t(6), constmix.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::DFunctionMixc), constmix.address());
    CPPUNIT_ASSERT_EQUAL(std::string("DFunctionMixc"), std::string(constmix.symbol()));

    // test selected template functions

    CFunction<float()> select("TemplateSelection0", (float(*)())(TestFunctions::TemplateSelection0));
    CPPUNIT_ASSERT_EQUAL(size_t(0), select.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::TemplateSelection0<float>), select.address());
    CPPUNIT_ASSERT_EQUAL(std::string("TemplateSelection0"), std::string(select.symbol()));

    // test multiple indirection layers

    CFunction<void(void*, void**, void***, float*, float**, float***)>
        mindirect("MultiPtrFunction", &TestFunctions::MultiPtrFunction);
    CPPUNIT_ASSERT_EQUAL(size_t(6), mindirect.size());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&TestFunctions::MultiPtrFunction), mindirect.address());
    CPPUNIT_ASSERT_EQUAL(std::string("MultiPtrFunction"), std::string(mindirect.symbol()));
}

void
TestFunctionSignature::testPrint()
{
    unittest_util::LLVMState state;
    std::ostringstream os;

    // test print empty function

    CFunction<void()> voidFunction("__void", nullptr);
    voidFunction.print(state.context(), os);
    CPPUNIT_ASSERT_EQUAL(std::string("void()"), os.str());
    os.str("");
    voidFunction.print(state.context(), os, voidFunction.symbol());
    CPPUNIT_ASSERT_EQUAL(std::string("void __void()"), os.str());
    os.str("");

    // test print scalar types

    CFunction<float(bool,int16_t,int32_t,int64_t,float,double)>
        scalars("__void", nullptr);
    std::string expected("float test(i1; i16; i32; i64; float; double)");
    scalars.print(state.context(), os, "test", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");
    expected = std::string("float test(bool; short; int; long; float; double)");
    scalars.print(state.context(), os, "test", true);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print scalar pointer types


    CFunction<void(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
        scalarptrs("__void", nullptr);
    expected = std::string("void(i1*; i16*; i32*; i64*; float*; double*)");
    scalarptrs.print(state.context(), os, "", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");
    // note that AX types ignore pointers
    expected = std::string("void(bool; short; int; long; float; double)");
    scalarptrs.print(state.context(), os, "", true);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print array pointer types

    CFunction<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
        arrayptrs("__void", nullptr);
    expected = std::string("void([1 x i1]*; [2 x i16]*; [3 x i32]*; [4 x i64]*; [5 x float]*; [6 x double]*)");
    arrayptrs.print(state.context(), os, "", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");
    // Some of these types are not supported AX types
    expected = std::string("void([1 x i1]*; [2 x i16]*; vec3i; [4 x i64]*; [5 x float]*; [6 x double]*)");
    arrayptrs.print(state.context(), os, "", true);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print mixture types

    CFunction<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
        mix("DFunctionMixc", &TestFunctions::DFunctionMixc);
    expected = std::string("double(i1; i16*; [1 x i32]*; i64; float*; [2 x double]*)");
    mix.print(state.context(), os, "", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");
    // Some of these types are not supported AX types
    expected = std::string("double(bool; short; [1 x i32]*; long; float; vec2d)");
    mix.print(state.context(), os, "", true);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print types from template selections

    CFunction<double()> select("TemplateSelection0", (double(*)())(TestFunctions::TemplateSelection0));
    expected = std::string("double()");
    select.print(state.context(), os, "", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print multi indirection types

    CFunction<void(void*, void**, void***, float*, float**, float***)>
        mindirect("MultiPtrFunction", &TestFunctions::MultiPtrFunction);
    // void pointers are pointers to structs containing a void member
    expected = std::string("void(i8*; i8**; i8***; float*; float**; float***)");
    mindirect.print(state.context(), os, "", false);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");
}

void
TestFunctionSignature::testTypeCreation()
{
    unittest_util::LLVMState state;

    llvm::Type* returnType = nullptr;
    std::vector<llvm::Type*> types;

    // test scalar arguments

    CFunction<int16_t(bool,int16_t,int32_t,int64_t,float,double)>
        scalars("IFunctionScalar", &TestFunctions::IFunctionScalar);
    returnType = scalars.types(types, state.context());
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<int16_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double>::get(state.context())));

    types.clear();

    // test scalar ptr arguments

    CFunction<int32_t(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
        scalarptrs("IFunctionScalarPtr", &TestFunctions::IFunctionScalarPtr);
    returnType = scalarptrs.types(types, state.context());
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<int32_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double*>::get(state.context())));

    types.clear();

    // test array ptr arguments

    CFunction<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
        arrayptrs("IFunctionArray", &TestFunctions::IFunctionArray);
    returnType = arrayptrs.types(types, state.context());
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<int64_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool(*)[1]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t(*)[2]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t(*)[3]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t(*)[4]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float(*)[5]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double(*)[6]>::get(state.context())));

    types.clear();

    // test argument mixture

    CFunction<float(bool,int16_t*,int32_t(*)[1],int64_t,float*,double(*)[2])>
        mix("FFunctionMix", &TestFunctions::FFunctionMix);
    returnType = mix.types(types, state.context());
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<float>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t(*)[1]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double(*)[2]>::get(state.context())));

    types.clear();

    // test argument mixture const

    CFunction<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
        constmix("DFunctionMixc", TestFunctions::DFunctionMixc);
    returnType = constmix.types(types, state.context());
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<double>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t(*)[1]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double(*)[2]>::get(state.context())));
}

// void
// TestFunctionSignature::testReturnOutputs()
// {
//     unittest_util::LLVMState state;

//     auto isAllocation = [](llvm::Value* v) -> bool {
//         return llvm::isa<llvm::AllocaInst>(v);
//     };

//     // create a dummy builder and give it something to write to

//     llvm::IRBuilder<> builder(state.scratchBlock());

//     // test zero arguments, zero output arguments, void return (no return)

//     Function::Ptr functionPtr;

//     CPPUNIT_ASSERT_NO_THROW(functionPtr
//         = CFunction<void()>::create(nullptr, "void"));
//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // test zero arguments, zero output arguments, int return

//     CPPUNIT_ASSERT_NO_THROW(functionPtr =
//         CFunction<int()>::create(nullptr, "void"));
//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // test 1 arguments, zero output arguments, int return

//     CPPUNIT_ASSERT_NO_THROW(functionPtr =
//         CFunction<int(int)>::create(nullptr, "void"));
//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // test 2 arguments, 1 output arguments, void return (no return)

//     CPPUNIT_ASSERT_NO_THROW(functionPtr =
//         CFunction<void(int*, float*)>::create(nullptr, "void", true));
//     CPPUNIT_ASSERT(functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())),
//         functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT(isAllocation(functionPtr->getOutputArgument(builder)));

//     // test 1 arguments, 1 output arguments, void return (no return)

//     CPPUNIT_ASSERT_NO_THROW(functionPtr =
//         CFunction<void(float(*)[3])>::create(nullptr, "void", true));
//     CPPUNIT_ASSERT(functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<float(*)[3]>::get(state.context())),
//         functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT(isAllocation(functionPtr->getOutputArgument(builder)));

//     // test 6 arguments, zero output arguments, float return

//     CPPUNIT_ASSERT_NO_THROW(functionPtr
//         = CFunction<float(bool,int16_t,int32_t,int64_t,float,double)>
//             ::create(nullptr, "__void"));
//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // test 6 arguments, 6 output arguments, void return (no return)

//     CPPUNIT_ASSERT_NO_THROW(functionPtr
//         = CFunction<void(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
//             ::create(nullptr, "__void", true));
//     CPPUNIT_ASSERT(functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<double*>::get(state.context())),
//         functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT(isAllocation(functionPtr->getOutputArgument(builder)));

//     // test 6 arguments, 2 output arguments, double return

//     CPPUNIT_ASSERT_NO_THROW(functionPtr = CFunction
//         <double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
//             ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc"));

//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // test 6 arguments, 3 output arguments, void return (no return)

//     CPPUNIT_ASSERT_NO_THROW(functionPtr
//         = CFunction<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
//             ::create(nullptr, "__void", 3));
//     CPPUNIT_ASSERT(functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<double(*)[6]>::get(state.context())),
//         functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT(isAllocation(functionPtr->getOutputArgument(builder)));

//     // test multi indirection arguments

//     CPPUNIT_ASSERT_NO_THROW(functionPtr
//         = CFunction<void**(void*, void**, void***, float*, float**, float***)>
//             ::create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction"));

//     CPPUNIT_ASSERT(!functionPtr->hasOutputArgument());
//     CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));

//     CPPUNIT_ASSERT_EQUAL((llvm::Type*)nullptr, functionPtr->getOutputType(state.context()));
//     CPPUNIT_ASSERT_EQUAL((llvm::Value*)nullptr, functionPtr->getOutputArgument(builder));

//     // invalid tests

//     CPPUNIT_ASSERT_THROW(CFunction
//         <void**(void*, void**, void***, float*, float**, float***)>
//             ::create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction", true),
//             openvdb::LLVMFunctionError);

//     CPPUNIT_ASSERT_THROW(CFunction
//         <double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
//             ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc", true),
//             openvdb::LLVMFunctionError);

//     // test 1 arguments, 1 output arguments, int return

//     CPPUNIT_ASSERT_THROW(CFunction<int(int*)>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);

//     // test 2 arguments, 1 output arguments, int return

//     CPPUNIT_ASSERT_THROW(CFunction<int(int*, float*)>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);

//     // test 1 arguments, 1 output arguments, float return

//     CPPUNIT_ASSERT_THROW(CFunction<float(float(*)[3])>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);

//     // test 0 arguments, output argument

//     CPPUNIT_ASSERT_THROW(CFunction<void()>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);
//     CPPUNIT_ASSERT_THROW(CFunction<int()>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);

//     // test invalid output return argument

//     CPPUNIT_ASSERT_THROW(CFunction<void(int)>::create(nullptr, "void", true),
//         openvdb::LLVMFunctionError);
// }

void
TestFunctionSignature::testMatching()
{
    unittest_util::LLVMState state;

    std::vector<llvm::Type*> types;

    // test void (no return), zero argument matching

    CFunction<void()> voidFunction("void", nullptr);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        voidFunction.match(types, state.context()));

    types.emplace_back(LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        voidFunction.match(types, state.context()));

    types.clear();

    // test int return, zero argument matching

    CFunction<int()> intf("void", nullptr);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        intf.match(types, state.context()));

    types.emplace_back(LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        intf.match(types, state.context()));

    types.clear();

    // test int return, 1 int argument matching

    CFunction<int32_t(int32_t)> intint("void", nullptr);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        intint.match(types, state.context()));

    types.resize(1);

    types[0] = LLVMType<void>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intint.match(types, state.context()));

    types[0] = LLVMType<void*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intint.match(types, state.context()));

    types[0] = LLVMType<int16_t>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Implicit,
        intint.match(types, state.context()));

    types[0] = LLVMType<int32_t>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        intint.match(types, state.context()));

    types[0] = LLVMType<int64_t>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Implicit,
        intint.match(types, state.context()));

    types[0] = LLVMType<float>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Implicit,
        intint.match(types, state.context()));

    types[0] = LLVMType<double>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Implicit,
        intint.match(types, state.context()));

    types[0] = LLVMType<int32_t*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intint.match(types, state.context()));

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intint.match(types, state.context()));

    types.clear();

    // test int return, 1 int* argument matching

    CFunction<int32_t(int32_t*)> intintptr("void", nullptr);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        intintptr.match(types, state.context()));

    types.resize(1);

    types[0] = LLVMType<void>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<void*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<int16_t*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<int32_t*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<int64_t*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));
    types[0] = LLVMType<float*>::get(state.context());

    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<double*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        intintptr.match(types, state.context()));

    types.clear();

    // test arrays

    CFunction<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
        arrayptrs("void", nullptr);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        arrayptrs.match(types, state.context()));

    types.resize(6);

    std::fill(types.begin(), types.end(), LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<void*>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<int32_t*>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<int32_t(*)[1]>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<double(*)[6]>::get(state.context()));
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    types[0] = LLVMType<bool(*)[1]>::get(state.context());
    types[1] = LLVMType<int16_t(*)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(*)[3]>::get(state.context());
    types[3] = LLVMType<int64_t(*)[4]>::get(state.context());
    types[4] = LLVMType<float(*)[5]>::get(state.context());
    types[5] = LLVMType<double(*)[6]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        arrayptrs.match(types, state.context()));

    types[0] = LLVMType<bool(*)[2]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    // @note - refuse to cast these vectors even though they're valid for implcit
    //         cast as they are not marked as readonly
    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    types[1] = LLVMType<int32_t(*)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(*)[3]>::get(state.context());
    types[3] = LLVMType<int32_t(*)[4]>::get(state.context());
    types[4] = LLVMType<int32_t(*)[5]>::get(state.context());
    types[5] = LLVMType<int32_t(*)[6]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    types[0] = LLVMType<bool(**)[1]>::get(state.context());
    types[1] = LLVMType<int16_t(**)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(**)[3]>::get(state.context());
    types[3] = LLVMType<int64_t(**)[4]>::get(state.context());
    types[4] = LLVMType<float(**)[5]>::get(state.context());
    types[5] = LLVMType<double(**)[6]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        arrayptrs.match(types, state.context()));

    types.clear();

    // test mixture

    CFunction<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
        mix("DFunctionMixc", &TestFunctions::DFunctionMixc);

    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        mix.match(types, state.context()));

    types.resize(6);

    types[0] = LLVMType<bool>::get(state.context());
    types[1] = LLVMType<int16_t*>::get(state.context());
    types[2] = LLVMType<int32_t(*)[1]>::get(state.context());
    types[3] = LLVMType<int64_t>::get(state.context());
    types[4] = LLVMType<float*>::get(state.context());
    types[5] = LLVMType<double(*)[2]>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        mix.match(types, state.context()));

    types[0] = LLVMType<double>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Implicit,
        mix.match(types, state.context()));

    types[1] = LLVMType<double*>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        mix.match(types, state.context()));

    types.clear();

    // test multi indirection arguments

    CFunction<void(void*, void**, void***, float*, float**, float***)>
        mindirect("MultiPtrFunction", &TestFunctions::MultiPtrFunction);

    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None,
        mindirect.match(types, state.context()));

    types.resize(6);

    types[0] = LLVMType<void*>::get(state.context());
    types[1] = LLVMType<void**>::get(state.context());
    types[2] = LLVMType<void***>::get(state.context());
    types[3] = LLVMType<float*>::get(state.context());
    types[4] = LLVMType<float**>::get(state.context());
    types[5] = LLVMType<float***>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Explicit,
        mindirect.match(types, state.context()));

    types[3] = LLVMType<float**>::get(state.context());
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::Size,
        mindirect.match(types, state.context()));
}

void
TestFunctionSignature::testCreateLLVMFunction()
{
    unittest_util::LLVMState state;

    llvm::Function* function = nullptr;
    llvm::FunctionType* functionType = nullptr;

    // test void empty args

    CFunction<void()> voidFunction("void", nullptr);
    function = voidFunction.create(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("void"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(0), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<void>::get(state.context()), functionType->getReturnType());

    // test int empty args

    CFunction<int()> intf("int", nullptr);
    function = intf.create(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("int"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(0), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<int>::get(state.context()), functionType->getReturnType());

    // test scalar

    CFunction<int16_t(bool,int16_t,int32_t,int64_t,float,double)> scalars("scalar", nullptr);
    function = scalars.create(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("scalar"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(6), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<int16_t>::get(state.context()), functionType->getReturnType());
    CPPUNIT_ASSERT_EQUAL(LLVMType<bool>::get(state.context()), functionType->getParamType(0));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int16_t>::get(state.context()), functionType->getParamType(1));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int32_t>::get(state.context()), functionType->getParamType(2));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int64_t>::get(state.context()), functionType->getParamType(3));
    CPPUNIT_ASSERT_EQUAL(LLVMType<float>::get(state.context()), functionType->getParamType(4));
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), functionType->getParamType(5));

    // test arrays

    CFunction<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
        arrayptrs("array", nullptr);
    function = arrayptrs.create(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("array"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(6), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<int64_t>::get(state.context()), functionType->getReturnType());
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<bool(*)[1]>::get(state.context())), functionType->getParamType(0));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int16_t(*)[2]>::get(state.context())), functionType->getParamType(1));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int32_t(*)[3]>::get(state.context())), functionType->getParamType(2));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int64_t(*)[4]>::get(state.context())), functionType->getParamType(3));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<float(*)[5]>::get(state.context())), functionType->getParamType(4));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<double(*)[6]>::get(state.context())), functionType->getParamType(5));

    // test mixture

    CFunction<double(const bool,int16_t* const,int32_t(*)[1],const int64_t,const float* const,const double(*)[2])>
        mix("mix", nullptr);
    function = mix.create(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("mix"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(6), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), functionType->getReturnType());
    CPPUNIT_ASSERT_EQUAL(LLVMType<bool>::get(state.context()), functionType->getParamType(0));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())), functionType->getParamType(1));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int32_t(*)[1]>::get(state.context())), functionType->getParamType(2));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int64_t>::get(state.context()), functionType->getParamType(3));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())), functionType->getParamType(4));
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<double(*)[2]>::get(state.context())), functionType->getParamType(5));
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
