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

#include "util.h"

#include <openvdb_ax/codegen/FunctionTypes.h>

#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include <string>

template <typename T>
using FunctionSignature = openvdb::ax::codegen::FunctionSignature<T>;
using FunctionSignatureBase = openvdb::ax::codegen::FunctionSignatureBase;
using FunctionBase = openvdb::ax::codegen::FunctionBase;
template <typename T>
using LLVMType = openvdb::ax::codegen::LLVMType<T>;

struct TestScalarFunction : public FunctionBase
{
    inline const std::string identifier() const override final { return std::string("TestScalarFunction"); }

    TestScalarFunction() : FunctionBase({
            FunctionSignature<void(double)>::create
                ((void(*)(double))(TestScalarFunction::scalar), std::string("tsfd")),
            FunctionSignature<void(float)>::create
                ((void(*)(float))(TestScalarFunction::scalar), std::string("tsff")),
            FunctionSignature<void(int64_t)>::create
                ((void(*)(int64_t))(TestScalarFunction::scalar), std::string("tsfi64")),
            FunctionSignature<void(int32_t)>::create
                ((void(*)(int32_t))(TestScalarFunction::scalar), std::string("tsfi32")),
            FunctionSignature<void(int16_t)>::create
                ((void(*)(int16_t))(TestScalarFunction::scalar), std::string("tsfi16")),
            FunctionSignature<void(bool)>::create
                ((void(*)(bool))(TestScalarFunction::scalar), std::string("tsfb"))
        }) {}

    template <typename T>
    static inline void scalar(const T) {}
};

struct TestSizeFunction : public FunctionBase
{
    inline const std::string identifier() const override final { return std::string("TestSizeFunction"); }

    TestSizeFunction() : FunctionBase({
            FunctionSignature<void()>::create
                ((void(*)())(TestSizeFunction::test), std::string("tsf")),
            FunctionSignature<void(double)>::create
                ((void(*)(double))(TestSizeFunction::test), std::string("tsfd")),
            FunctionSignature<void(double, double)>::create
                ((void(*)(double, double))(TestSizeFunction::test), std::string("tsfdd"))
        }) {}

    static inline void test() {}
    static inline void test(double) {}
    static inline void test(double, double) {}
};

struct TestMultiFunction : public FunctionBase
{
    inline const std::string identifier() const override final { return std::string("TestMultiFunction"); }

    TestMultiFunction() : FunctionBase({
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test0),  // empty function
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test1),  // void(int32) - no return
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test2),  // void(double, double) - no return
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test3),  // int(int32, double)
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(TestMultiFunction::test4),  // void(int32, double, double*) - output arg double
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test5),  // void(int32[1]) - no return
        DECLARE_FUNCTION_SIGNATURE(TestMultiFunction::test6),  // void(int32[2]) - no return
        DECLARE_FUNCTION_SIGNATURE_OUTPUT(TestMultiFunction::test6)   // void(int32[2]) - output arg int32[2]
    }) {}

    static inline void test0() {}
    static inline void test1(int32_t) {}
    static inline void test2(double, double) {}
    static inline int test3(int32_t, double) { return int(); }
    static inline void test4(int32_t, double, double*) {}
    static inline void test5(int32_t(*)[1]) {}
    static inline void test6(int32_t(*)[2]) {}
};

struct TestIRFunction : public FunctionBase
{
    inline const std::string identifier() const override final { return std::string("TestIRFunction"); }

    TestIRFunction() : FunctionBase({
            FunctionSignature<double(double)>::create(nullptr, std::string("double")),
            FunctionSignature<float(float)>::create(nullptr, std::string("float")),
            FunctionSignature<int32_t(int32_t)>::create(nullptr, std::string("int32")),
            FunctionSignature<void(int8_t)>::create(nullptr, std::string("int8"))
        }) {}

    inline llvm::Value*
    generate(const std::vector<llvm::Value*>& args,
             const std::unordered_map<std::string, llvm::Value*>&,
             llvm::IRBuilder<>& builder) const override final
    {
        if (args.empty())     return nullptr;
        if (args.size() != 1) return nullptr;

        if (args[0]->getType()->isDoubleTy())     return llvm::ConstantFP::get(LLVMType<double>::get(builder.getContext()), 1.0);
        if (args[0]->getType()->isFloatTy())      return llvm::ConstantFP::get(LLVMType<float>::get(builder.getContext()), 1.0f);
        if (args[0]->getType()->isIntegerTy(32))  return builder.getInt32(1);
        if (args[0]->getType()->isIntegerTy(8))   return builder.getInt32(1); // unexpected return type for int8

        return nullptr;
    };
};

class TestFunctionBase : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestFunctionBase);
    CPPUNIT_TEST(testCreate);
    CPPUNIT_TEST(testMatching);
    CPPUNIT_TEST(testExecuteCall);
    CPPUNIT_TEST(testGenerate);
    CPPUNIT_TEST_SUITE_END();

    void testCreate();
    void testMatching();
    void testExecuteCall();
    void testGenerate();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFunctionBase);

void
TestFunctionBase::testCreate()
{
    unittest_util::LLVMState state;

    FunctionBase::Ptr function;
    function.reset(new TestScalarFunction());

    CPPUNIT_ASSERT_EQUAL(std::string("TestScalarFunction"), function->identifier());

    const std::vector<FunctionSignatureBase::Ptr>* list = &function->list();
    CPPUNIT_ASSERT_EQUAL(size_t(6), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfd"), (*list)[0]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsff"), (*list)[1]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi64"), (*list)[2]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi32"), (*list)[3]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi16"), (*list)[4]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfb"), (*list)[5]->symbolName());

    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[3]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[4]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[5]->size());

    CPPUNIT_ASSERT(!(*list)[0]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[1]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[2]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[3]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[4]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[5]->hasReturnValue(state.context()));

    CPPUNIT_ASSERT(!(*list)[0]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[1]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[2]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[3]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[4]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[5]->hasOutputArgument());

    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<double>), (*list)[0]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<float>), (*list)[1]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<int64_t>), (*list)[2]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<int32_t>), (*list)[3]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<int16_t>), (*list)[4]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestScalarFunction::scalar<bool>), (*list)[5]->functionPointer());

    //

    function.reset(new TestSizeFunction());

    CPPUNIT_ASSERT_EQUAL(std::string("TestSizeFunction"), function->identifier());

    list = &function->list();
    CPPUNIT_ASSERT_EQUAL(size_t(3), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("tsf"), (*list)[0]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfd"), (*list)[1]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfdd"), (*list)[2]->symbolName());

    CPPUNIT_ASSERT_EQUAL(size_t(0), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[2]->size());

    CPPUNIT_ASSERT(!(*list)[0]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[1]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[2]->hasReturnValue(state.context()));

    CPPUNIT_ASSERT(!(*list)[0]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[1]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[2]->hasOutputArgument());

    auto test0 = static_cast<void(*)()>(&TestSizeFunction::test);
    auto test1 = static_cast<void(*)(double)>(&TestSizeFunction::test);
    auto test2 = static_cast<void(*)(double, double)>(&TestSizeFunction::test);

    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(test0), (*list)[0]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(test1), (*list)[1]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(test2), (*list)[2]->functionPointer());

    //

    function.reset(new TestMultiFunction());

    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction"), function->identifier());

    list = &function->list();
    CPPUNIT_ASSERT_EQUAL(size_t(8), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test0"), (*list)[0]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test1"), (*list)[1]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test2"), (*list)[2]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test3"), (*list)[3]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test4"), (*list)[4]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test5"), (*list)[5]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test6"), (*list)[6]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction::test6"), (*list)[7]->symbolName());

    CPPUNIT_ASSERT_EQUAL(size_t(0), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[3]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(3), (*list)[4]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[5]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[6]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[7]->size());

    CPPUNIT_ASSERT(!(*list)[0]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[1]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[2]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT((*list)[3]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[4]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[5]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[6]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[7]->hasReturnValue(state.context()));

    CPPUNIT_ASSERT(!(*list)[0]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[1]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[2]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[3]->hasOutputArgument());
    CPPUNIT_ASSERT((*list)[4]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[5]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[6]->hasOutputArgument());
    CPPUNIT_ASSERT((*list)[7]->hasOutputArgument());

    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test0), (*list)[0]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test1), (*list)[1]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test2), (*list)[2]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test3), (*list)[3]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test4), (*list)[4]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test5), (*list)[5]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test6), (*list)[6]->functionPointer());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<void(*)()>(&TestMultiFunction::test6), (*list)[7]->functionPointer());


    //

    function.reset(new TestIRFunction());

    CPPUNIT_ASSERT_EQUAL(std::string("TestIRFunction"), function->identifier());

    list = &function->list();
    CPPUNIT_ASSERT_EQUAL(size_t(4), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("double"), (*list)[0]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("float"), (*list)[1]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("int32"), (*list)[2]->symbolName());
    CPPUNIT_ASSERT_EQUAL(std::string("int8"), (*list)[3]->symbolName());

    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[3]->size());

    CPPUNIT_ASSERT((*list)[0]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT((*list)[1]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT((*list)[2]->hasReturnValue(state.context()));
    CPPUNIT_ASSERT(!(*list)[3]->hasReturnValue(state.context()));

    CPPUNIT_ASSERT(!(*list)[0]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[1]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[2]->hasOutputArgument());
    CPPUNIT_ASSERT(!(*list)[3]->hasOutputArgument());

    CPPUNIT_ASSERT((*list)[0]->functionPointer() == nullptr);
    CPPUNIT_ASSERT((*list)[1]->functionPointer() == nullptr);
    CPPUNIT_ASSERT((*list)[2]->functionPointer() == nullptr);
    CPPUNIT_ASSERT((*list)[3]->functionPointer() == nullptr);
}

void
TestFunctionBase::testMatching()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionBase::Ptr function;
    function.reset(new TestScalarFunction());

    const std::vector<FunctionSignatureBase::Ptr>* list = &function->list();

    std::vector<llvm::Type*> types;
    FunctionBase::FunctionMatch match;

    types.resize(1);

    // test explicit matching

    types[0] = LLVMType<bool>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[5], match.first);
    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[5], match.first);

    //

    types[0] = LLVMType<int16_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[4], match.first);

    //

    types[0] = LLVMType<int32_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[3], match.first);

    //

    types[0] = LLVMType<int64_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    //

    types[0] = LLVMType<float>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[1], match.first);

    //

    types[0] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);

    //

    types[0] = LLVMType<int64_t>::get(state.context()); // int64_t

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    // test unsigned integers automatic type creation - these are not supported in the
    // language however can be constructed from the API. The function framework does
    // not differentiate between signed and unsigned integers

    types[0] = LLVMType<uint64_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    // test implicit matching - types should match to the first available castable signature
    // which is always the void(double) "tsfd" function for all provided scalars

    types[0] = LLVMType<int8_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);

    types.clear();

    // test invalid matching - Size matching returns the first function which matched
    // the size

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None == match.second);
    CPPUNIT_ASSERT(!match.first);
    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None == match.second);
    CPPUNIT_ASSERT(!match.first);

    //

    types.emplace_back(LLVMType<bool*>::get(state.context()));

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size == match.second);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);
    CPPUNIT_ASSERT(match.first);
    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size == match.second);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);
    CPPUNIT_ASSERT(match.first);

    //

    types[0] = LLVMType<bool[1]>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size == match.second);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);
    CPPUNIT_ASSERT(match.first);
    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Size == match.second);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);
    CPPUNIT_ASSERT(match.first);

    //

    types[0] = LLVMType<bool>::get(state.context());
    types.emplace_back(LLVMType<bool>::get(state.context()));

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None == match.second);
    CPPUNIT_ASSERT(!match.first);
    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None == match.second);
    CPPUNIT_ASSERT(!match.first);

    // Test varying argument size function

    // test explicit matching

    function.reset(new TestSizeFunction());
    list = &function->list();

    types.resize(2);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    //

    types.resize(1);

    types[0] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[1], match.first);

    //

    types.clear();

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);

    // Test implicit matching

    types.resize(2);

    types[0] = LLVMType<float>::get(state.context());
    types[1] = LLVMType<int32_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    // Test non matching

    types.resize(3);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());
    types[2] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::None == match.second);
    CPPUNIT_ASSERT(!match.first);

    // Test multi function

    function.reset(new TestMultiFunction());
    list = &function->list();

    // test explicit/implicit matching

    types.clear();

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[0], match.first);

    //

    types.resize(2);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    //

    types[0] = LLVMType<int32_t>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[3], match.first);

    //

    types[0] = LLVMType<int32_t>::get(state.context());
    types[1] = LLVMType<int32_t>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[2], match.first);

    // add output arguments

    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Implicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[4], match.first);

    //

    types.resize(1);

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[5], match.first);

    //

    types[0] = LLVMType<int32_t(*)[2]>::get(state.context());

    match = function->match(types, state.context(), /*add output arguments*/false);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[6], match.first);

    // add output arguments

    types.clear();

    match = function->match(types, state.context(), /*add output arguments*/true);
    CPPUNIT_ASSERT(FunctionSignatureBase::SignatureMatch::Explicit == match.second);
    CPPUNIT_ASSERT(match.first);
    CPPUNIT_ASSERT_EQUAL((*list)[7], match.first);
}

using Globals = std::unordered_map<std::string, llvm::Value*>;

void
TestFunctionBase::testExecuteCall()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionBase::Ptr function;
    function.reset(new TestScalarFunction());

    const std::vector<FunctionSignatureBase::Ptr>* list = &function->list();

    // test execution

    llvm::Value* result = nullptr;
    llvm::CallInst* call = nullptr;
    llvm::Function* target = nullptr;
    std::vector<llvm::Value*> args, results;

    // test invalid arguments throws

    CPPUNIT_ASSERT_THROW(function->execute(/*args*/{}, Globals(), builder, &results),
        openvdb::LLVMFunctionError);
    CPPUNIT_ASSERT(results.empty());

    args.resize(1);

    // test llvm function calls - execute and get the called function. check this is already
    // inserted into the module and is expected llvm::Function using toLLVMFunction on the
    // expected function signature

    args[0] = builder.getTrue();
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[5]->toLLVMFunction(state.module()), target);

    //

    args[0] = builder.getInt16(1);
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[4]->toLLVMFunction(state.module()), target);

    //

    args[0] = builder.getInt32(1);
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[3]->toLLVMFunction(state.module()), target);

    //

    args[0] = builder.getInt64(1);
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[2]->toLLVMFunction(state.module()), target);

    //

    args[0] = llvm::ConstantFP::get(LLVMType<float>::get(state.context()), 1.0f);
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[1]->toLLVMFunction(state.module()), target);

    //

    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);
    result = function->execute(args, Globals(), builder, &results);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[0]->toLLVMFunction(state.module()), target);


    // Test multi function with output arguments and results

    function.reset(new TestMultiFunction());

    list = &function->list();
    results.clear();
    args.clear();

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[0]->toLLVMFunction(state.module()), target);

    // test empty with output arg

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/true);

    CPPUNIT_ASSERT_EQUAL(size_t(1), results.size());
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<int32_t(*)[2]>::get(state.context())), results.front()->getType());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[6]->toLLVMFunction(state.module()), target);

    //

    results.clear();
    args.resize(1);

    args[0] = builder.CreateAlloca(llvm::ArrayType::get(LLVMType<int32_t>::get(state.context()), 1));

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[5]->toLLVMFunction(state.module()), target);

    //

    args[0] = builder.CreateAlloca(llvm::ArrayType::get(LLVMType<int32_t>::get(state.context()), 2));

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[6]->toLLVMFunction(state.module()), target);

    //

    args.resize(2);

    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);
    args[1] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[2]->toLLVMFunction(state.module()), target);

    // test with output arg

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/true);

    CPPUNIT_ASSERT_EQUAL(size_t(1), results.size());
    CPPUNIT_ASSERT_EQUAL(llvm::cast<llvm::Type>(LLVMType<double*>::get(state.context())), results.front()->getType());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[4]->toLLVMFunction(state.module()), target);
    CPPUNIT_ASSERT_EQUAL(LLVMType<void>::get(state.context()), target->getReturnType());
}

void
TestFunctionBase::testGenerate()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionBase::Ptr function;
    function.reset(new TestIRFunction());

    llvm::Value* result = nullptr;
    std::vector<llvm::Value*> args, results;

    // test invalid arguments throws

    CPPUNIT_ASSERT_THROW(function->execute(/*args*/{}, Globals(), builder, &results),
        openvdb::LLVMFunctionError);
    CPPUNIT_ASSERT(results.empty());

    // test IR generation

    args.resize(1);
    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(!llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), result->getType());

    //

    args[0] = llvm::ConstantFP::get(LLVMType<float>::get(state.context()), 1.0);

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(!llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<float>::get(state.context()), result->getType());

    //

    args[0] = builder.getInt32(1);

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(!llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int32_t>::get(state.context()), result->getType());

    // test values are cast with implicit matching

    args[0] = builder.getInt16(1);

    result = function->execute(args, Globals(), builder, &results, /*add output arguments*/false);

    CPPUNIT_ASSERT(results.empty());
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(!llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), result->getType());

    // test throw on unexpected return type

    args[0] = builder.getInt8(1);

    CPPUNIT_ASSERT_THROW(function->execute(/*args*/{}, Globals(), builder, &results),
        openvdb::LLVMFunctionError);
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
