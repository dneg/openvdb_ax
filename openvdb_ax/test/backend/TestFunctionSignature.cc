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

#include "util.h"

#include <openvdb_ax/codegen/FunctionTypes.h>

#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include <string>

template <typename T>
using FunctionSignature = openvdb::ax::codegen::FunctionSignature<T>;
using FunctionSignatureBase = openvdb::ax::codegen::FunctionSignatureBase;
template <typename T>
using LLVMType = openvdb::ax::codegen::LLVMType<T>;

class TestFunctionSignature : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestFunctionSignature);
    CPPUNIT_TEST(testCreate);
    CPPUNIT_TEST(testPrint);
    CPPUNIT_TEST(testTypeCreation);
    CPPUNIT_TEST(testReturnOutputs);
    CPPUNIT_TEST(testMatching);
    CPPUNIT_TEST(testLLVMFunction);
    CPPUNIT_TEST_SUITE_END();

    void testCreate();
    void testPrint();
    void testTypeCreation();
    void testReturnOutputs();
    void testMatching();
    void testLLVMFunction();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFunctionSignature);

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

    static void** MultiPtrFunction(void*, void**, void***, float*, float**, float***) { return nullptr; }
};


void
TestFunctionSignature::testCreate()
{
    FunctionSignature<void()> voidFunction(nullptr, "");
    CPPUNIT_ASSERT_EQUAL(size_t(0), voidFunction.size());
    CPPUNIT_ASSERT(!voidFunction.functionPointer());
    CPPUNIT_ASSERT_EQUAL(voidFunction.symbolName(), std::string(""));

    FunctionSignatureBase::Ptr functionPtr;

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void()>::create(nullptr, "__void"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(size_t(0), functionPtr->size());
    CPPUNIT_ASSERT(!functionPtr->functionPointer());
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("__void"));

    // test static void function

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void()>::create(&TestFunctions::SVFunction, "SVFunction"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("SVFunction"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::SVFunction));

    // test inline static void function

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void()>::create(&TestFunctions::SIVFunction, "SIVFunction"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("SIVFunction"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::SIVFunction));

    // test scalar arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int16_t(bool,int16_t,int32_t,int64_t,float,double)>
            ::create(&TestFunctions::IFunctionScalar, "IFunctionScalar"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("IFunctionScalar"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::IFunctionScalar));

    // test scalar ptr arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int32_t(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
            ::create(&TestFunctions::IFunctionScalarPtr, "IFunctionScalarPtr"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("IFunctionScalarPtr"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::IFunctionScalarPtr));

    // test array ptr arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(&TestFunctions::IFunctionArray, "IFunctionArray"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("IFunctionArray"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::IFunctionArray));

    // test argument mixture

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<float(bool,int16_t*,int32_t(*)[1],int64_t,float*,double(*)[2])>
            ::create(&TestFunctions::FFunctionMix, "FFunctionMix"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("FFunctionMix"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::FFunctionMix));

    // test argument mixture const

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("DFunctionMixc"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::DFunctionMixc));

    // test selected template functions

    CPPUNIT_ASSERT_NO_THROW(functionPtr = FunctionSignature<float()>::
        create((float(*)())(TestFunctions::TemplateSelection0), "TemplateSelection0"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("TemplateSelection0"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::TemplateSelection0<float>));

    // test multiple indirection layers

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void**(void*, void**, void***, float*, float**, float***)>::
            create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction"));
    CPPUNIT_ASSERT(functionPtr);
    CPPUNIT_ASSERT_EQUAL(functionPtr->symbolName(), std::string("MultiPtrFunction"));
    CPPUNIT_ASSERT_EQUAL(functionPtr->functionPointer(),
        reinterpret_cast<void*>(&TestFunctions::MultiPtrFunction));

    // test non matching - The function signature generator will allow compilation
    // of void() templates with differing layers of indirection, but fail at runtime on
    // instantiation if the exact types do not match

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SFFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SIFFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SVpFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SIVpFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SFpFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::
        create(&TestFunctions::SIFpFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void(void*, void**, void***, float*, float**, float***)>::
        create(&TestFunctions::MultiPtrFunction, "_"), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void*(void*, void**, void***, float*, float**, float***)>::
        create(&TestFunctions::MultiPtrFunction, "_"), openvdb::LLVMFunctionError);
}

void
TestFunctionSignature::testPrint()
{
    unittest_util::LLVMState state;
    std::ostringstream os;

    FunctionSignature<void()> voidFunction(nullptr, "__void");
    voidFunction.print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(std::string("void()"), os.str());
    os.str("");

    voidFunction.print(state.context(), voidFunction.symbolName(),  os);
    CPPUNIT_ASSERT_EQUAL(std::string("void __void()"), os.str());
    os.str("");

    FunctionSignatureBase::Ptr functionPtr;

    // test print empty function

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void()>::create(nullptr, "__void"));
     functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(std::string("void()"), os.str());
    os.str("");
    functionPtr->print(state.context(), functionPtr->symbolName(),  os);
    CPPUNIT_ASSERT_EQUAL(std::string("void __void()"), os.str());
    os.str("");

    // test print scalar types

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<float(bool,int16_t,int32_t,int64_t,float,double)>
            ::create(nullptr, "__void"));
     std::string expected("float test(i1; i16; i32; i64; float; double)");
    functionPtr->print(state.context(), "test", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print scalar pointer types

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
            ::create(nullptr, "__void"));
    expected = std::string("void(i1*; i16*; i32*; i64*; float*; double*)");
    functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print array pointer types

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(nullptr, "__void"));
    expected = std::string("void([1 x i1]*; [2 x i16]*; [3 x i32]*; [4 x i64]*; [5 x float]*; [6 x double]*)");
    functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print mixture types

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc"));
    expected = std::string("double(i1; i16*; [1 x i32]*; i64; float*; [2 x double]*)");
    functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print types from template selections

    CPPUNIT_ASSERT_NO_THROW(functionPtr = FunctionSignature<double()>::
        create((double(*)())(TestFunctions::TemplateSelection0), "TemplateSelection0"));
    expected = std::string("double()");
    functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");

    // test print multi indirection types

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void**(void*, void**, void***, float*, float**, float***)>::
            create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction"));
    expected = std::string("void**(void*; void**; void***; float*; float**; float***)");
    functionPtr->print(state.context(), "", os);
    CPPUNIT_ASSERT_EQUAL(expected, os.str());
    os.str("");;
}

void
TestFunctionSignature::testTypeCreation()
{
    unittest_util::LLVMState state;

    llvm::Type* returnType = nullptr;
    std::vector<llvm::Type*> types;

    FunctionSignatureBase::Ptr functionPtr;

    // test scalar arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int16_t(bool,int16_t,int32_t,int64_t,float,double)>
            ::create(&TestFunctions::IFunctionScalar, "IFunctionScalar"));
    returnType = functionPtr->toLLVMTypes(state.context(), &types);
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int32_t(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
            ::create(&TestFunctions::IFunctionScalarPtr, "IFunctionScalarPtr"));
    returnType = functionPtr->toLLVMTypes(state.context(), &types);
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(&TestFunctions::IFunctionArray, "IFunctionArray"));
    returnType = functionPtr->toLLVMTypes(state.context(), &types);
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<float(bool,int16_t*,int32_t(*)[1],int64_t,float*,double(*)[2])>
            ::create(&TestFunctions::FFunctionMix, "FFunctionMix"));
    returnType = functionPtr->toLLVMTypes(state.context(), &types);
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc"));
    returnType = functionPtr->toLLVMTypes(state.context(), &types);
    CPPUNIT_ASSERT_EQUAL(returnType, llvm::cast<llvm::Type>(LLVMType<double>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t(*)[1]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double(*)[2]>::get(state.context())));
}

void
TestFunctionSignature::testReturnOutputs()
{
    unittest_util::LLVMState state;
    std::vector<llvm::Value*> values;
    std::vector<llvm::Type*> types;

    auto isAllocation = [](llvm::Value*& v) {
        CPPUNIT_ASSERT(llvm::isa<llvm::AllocaInst>(v));
    };

    // create a dummy builder and give it something to write to

    llvm::IRBuilder<> builder(state.scratchBlock());

    // test zero arguments, zero output arguments, void return (no return)

    FunctionSignatureBase::Ptr functionPtr;

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void()>::create(nullptr, "void"));
    CPPUNIT_ASSERT(!functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(0), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT(types.empty());
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT(values.empty());

    // test zero arguments, zero output arguments, int return

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int()>::create(nullptr, "void"));
    CPPUNIT_ASSERT(!functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(1), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT(types.empty());
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT(values.empty());

    // test 1 arguments, zero output arguments, int return

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int(int)>::create(nullptr, "void"));
    CPPUNIT_ASSERT(!functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(1), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT(types.empty());
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT(values.empty());

    // test 1 arguments, 1 output arguments, int return

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int(int*)>::create(nullptr, "void", 1));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(2), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(1), types.size());
    CPPUNIT_ASSERT_EQUAL(types.front(), llvm::cast<llvm::Type>(LLVMType<int*>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(1), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 2 arguments, 2 output arguments, int return

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int(int*, float*)>::create(nullptr, "void", 2));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(3), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(2), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<int*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(2), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 2 arguments, 2 output arguments, void return (no return)

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void(int*, float*)>::create(nullptr, "void", 2));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(2), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(2), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<int*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(2), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 1 arguments, 1 output arguments, void return (no return)

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<void(float(*)[3])>::create(nullptr, "void", 1));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(1), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(1), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<float(*)[3]>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(1), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 1 arguments, 1 output arguments, float return

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<float(float(*)[3])>::create(nullptr, "void", 1));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(2), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(1), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<float(*)[3]>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(1), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 6 arguments, zero output arguments, float return

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<float(bool,int16_t,int32_t,int64_t,float,double)>
            ::create(nullptr, "__void"));
    CPPUNIT_ASSERT(!functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(1), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT(types.empty());
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT(values.empty());

    // test 6 arguments, 6 output arguments, void return (no return)

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool*,int16_t*,int32_t*,int64_t*,float*,double*)>
            ::create(nullptr, "__void", 6));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(6), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<bool*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<int16_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<int32_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<int64_t*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<double*>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(6), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 6 arguments, 2 output arguments, double return

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc", 2));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(3), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(2), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<double(*)[2]>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(2), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    types.clear(); values.clear();

    // test 6 arguments, 3 output arguments, void return (no return)

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(nullptr, "__void", 3));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(!functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(3), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(3), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<int64_t(*)[4]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<float(*)[5]>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<double(*)[6]>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(3), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

     types.clear(); values.clear();

    // test multi indirection arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void**(void*, void**, void***, float*, float**, float***)>
            ::create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction", 6));
    CPPUNIT_ASSERT(functionPtr->hasOutputArguments());
    CPPUNIT_ASSERT(functionPtr->hasReturnValue(state.context()));
    CPPUNIT_ASSERT_EQUAL(size_t(7), functionPtr->numReturnValues(state.context()));

    functionPtr->appendOutputTypes(types, state.context());
    CPPUNIT_ASSERT_EQUAL(size_t(6), types.size());
    CPPUNIT_ASSERT_EQUAL(types[0], llvm::cast<llvm::Type>(LLVMType<void*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[1], llvm::cast<llvm::Type>(LLVMType<void**>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[2], llvm::cast<llvm::Type>(LLVMType<void***>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[3], llvm::cast<llvm::Type>(LLVMType<float*>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[4], llvm::cast<llvm::Type>(LLVMType<float**>::get(state.context())));
    CPPUNIT_ASSERT_EQUAL(types[5], llvm::cast<llvm::Type>(LLVMType<float***>::get(state.context())));
    functionPtr->appendOutputArguments(values, builder);
    CPPUNIT_ASSERT_EQUAL(size_t(6), values.size());
    std::for_each(values.begin(), values.end(), isAllocation);

    //

    // test incompatible output types (either too many have been specified, or the ones that have
    // been are not pointers)

    CPPUNIT_ASSERT_THROW(FunctionSignature<void()>::create(nullptr, "void", 1),
        openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(FunctionSignature<void(int*, int*)>::create(nullptr, "void", 3),
        openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc", 7), openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc", 4));
    CPPUNIT_ASSERT_THROW(functionPtr->appendOutputArguments(values, builder),
        openvdb::LLVMFunctionError);

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool*,int16_t*,int32_t*,int64_t*,float*,double)>
            ::create(nullptr, "__void", 6));
    CPPUNIT_ASSERT_THROW(functionPtr->appendOutputArguments(values, builder),
        openvdb::LLVMFunctionError);
}

void
TestFunctionSignature::testMatching()
{
    unittest_util::LLVMState state;

    std::vector<llvm::Type*> types;

    FunctionSignatureBase::Ptr functionPtr;

    // test void (no return), zero argument matching

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void()>::create(nullptr, "void"));
    CPPUNIT_ASSERT(functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(1));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types.emplace_back(LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));

    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test int return, zero argument matching

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int()>::create(nullptr, "void"));
    CPPUNIT_ASSERT(functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(1));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types.emplace_back(LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));

    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test int return, 1 int argument matching

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int32_t(int32_t)>::create(nullptr, "void"));
    CPPUNIT_ASSERT(functionPtr->sizeMatch(1));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(2));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.resize(1);

    types[0] = LLVMType<void>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<void*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int16_t>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int64_t>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));
    types[0] = LLVMType<float>::get(state.context());

    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<double>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test int return, 1 int* argument matching

    CPPUNIT_ASSERT_NO_THROW(functionPtr =
        FunctionSignature<int32_t(int32_t*)>::create(nullptr, "void"));
    CPPUNIT_ASSERT(functionPtr->sizeMatch(1));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(2));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.resize(1);

    types[0] = LLVMType<void>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<void*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int16_t*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t*>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int64_t*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));
    types[0] = LLVMType<float*>::get(state.context());

    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<double*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test arrays

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(nullptr, "void"));
    CPPUNIT_ASSERT(functionPtr->sizeMatch(6));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.resize(6);

    std::fill(types.begin(), types.end(), LLVMType<void>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<void*>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<int32_t*>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<int32_t(*)[1]>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    std::fill(types.begin(), types.end(), LLVMType<double(*)[6]>::get(state.context()));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<bool(*)[1]>::get(state.context());
    types[1] = LLVMType<int16_t(*)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(*)[3]>::get(state.context());
    types[3] = LLVMType<int64_t(*)[4]>::get(state.context());
    types[4] = LLVMType<float(*)[5]>::get(state.context());
    types[5] = LLVMType<double(*)[6]>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<bool(*)[2]>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());
    types[1] = LLVMType<int32_t(*)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(*)[3]>::get(state.context());
    types[3] = LLVMType<int32_t(*)[4]>::get(state.context());
    types[4] = LLVMType<int32_t(*)[5]>::get(state.context());
    types[5] = LLVMType<int32_t(*)[6]>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<bool(**)[1]>::get(state.context());
    types[1] = LLVMType<int16_t(**)[2]>::get(state.context());
    types[2] = LLVMType<int32_t(**)[3]>::get(state.context());
    types[3] = LLVMType<int64_t(**)[4]>::get(state.context());
    types[4] = LLVMType<float(**)[5]>::get(state.context());
    types[5] = LLVMType<double(**)[6]>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test mixture

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool, int16_t* const, int32_t(*)[1], const int64_t, const float* const, const double(*)[2])>
            ::create(&TestFunctions::DFunctionMixc, "DFunctionMixc", 2));

    CPPUNIT_ASSERT(functionPtr->sizeMatch(6));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.resize(6);

    types[0] = LLVMType<bool>::get(state.context());
    types[1] = LLVMType<int16_t*>::get(state.context());
    types[2] = LLVMType<int32_t(*)[1]>::get(state.context());
    types[3] = LLVMType<int64_t>::get(state.context());
    types[4] = LLVMType<float*>::get(state.context());
    types[5] = LLVMType<double(*)[2]>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types[0] = LLVMType<double>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit ==
        functionPtr->match(types, state.context()));

    types[1] = LLVMType<double*>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));

    types.clear();

    // test multi indirection arguments

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void**(void*, void**, void***, float*, float**, float***)>
            ::create(&TestFunctions::MultiPtrFunction, "MultiPtrFunction", 6));

    CPPUNIT_ASSERT(functionPtr->sizeMatch(6));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(0));
    CPPUNIT_ASSERT(!functionPtr->sizeMatch(-1));
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None ==
        functionPtr->match(types, state.context()));

    types.resize(6);

    types[0] = LLVMType<void*>::get(state.context());
    types[1] = LLVMType<void**>::get(state.context());
    types[2] = LLVMType<void***>::get(state.context());
    types[3] = LLVMType<float*>::get(state.context());
    types[4] = LLVMType<float**>::get(state.context());
    types[5] = LLVMType<float***>::get(state.context());
    CPPUNIT_ASSERT(functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit ==
        functionPtr->match(types, state.context()));

    types[3] = LLVMType<float**>::get(state.context());
    CPPUNIT_ASSERT(!functionPtr->implicitMatch(types, state.context()));
    CPPUNIT_ASSERT(!functionPtr->explicitMatch(types, state.context()));
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size ==
        functionPtr->match(types, state.context()));
}

void
TestFunctionSignature::testLLVMFunction()
{
    unittest_util::LLVMState state;

    FunctionSignatureBase::Ptr functionPtr;
    llvm::Function* function = nullptr;
    llvm::FunctionType* functionType = nullptr;

    // test void empty args

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<void()>::create(nullptr, "void"));
    function = functionPtr->toLLVMFunction(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("void"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(0), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<void>::get(state.context()), functionType->getReturnType());

    // test int empty args

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int()>::create(nullptr, "int"));
    function = functionPtr->toLLVMFunction(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("int"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(0), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<int>::get(state.context()), functionType->getReturnType());

    // test int empty args

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int()>::create(nullptr, "int"));
    function = functionPtr->toLLVMFunction(state.module());
    CPPUNIT_ASSERT(function);
    CPPUNIT_ASSERT_EQUAL(function, state.module().getFunction("int"));
    functionType = function->getFunctionType();
    CPPUNIT_ASSERT(functionType);
    CPPUNIT_ASSERT_EQUAL(unsigned(0), functionType->getNumParams());
    CPPUNIT_ASSERT_EQUAL(LLVMType<int>::get(state.context()), functionType->getReturnType());

    // test scalar

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int16_t(bool,int16_t,int32_t,int64_t,float,double)>::create(nullptr, "scalar"));
    function = functionPtr->toLLVMFunction(state.module());
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<int64_t(bool(*)[1],int16_t(*)[2],int32_t(*)[3],int64_t(*)[4],float(*)[5],double(*)[6])>
            ::create(nullptr, "array"));
    function = functionPtr->toLLVMFunction(state.module());
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

    CPPUNIT_ASSERT_NO_THROW(functionPtr
        = FunctionSignature<double(const bool,int16_t* const,int32_t(*)[1],const int64_t,const float* const,const double(*)[2])>
            ::create(nullptr, "mix"));
    function = functionPtr->toLLVMFunction(state.module());
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

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
