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
using CFunctionBase = openvdb::ax::codegen::CFunctionBase;
using IRFunctionBase = openvdb::ax::codegen::IRFunctionBase;
using FunctionGroup = openvdb::ax::codegen::FunctionGroup;
using FunctionBuilder = openvdb::ax::codegen::FunctionBuilder;
template <typename T>
using LLVMType = openvdb::ax::codegen::LLVMType<T>;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// Framework methods for the subsequent unit tests

static inline void test() {}
template <typename T1, typename RetT> static inline RetT test1(T1) { return RetT(); }
template <typename T1> static inline void test1(T1) {}

template <typename T1, typename T2, typename RetT> static inline RetT test2(T1, T2) { return RetT(); }
template <typename T1, typename T2> static inline void test2(T1, T2) {}

template <typename T1, typename T2, typename T3, typename RetT> static inline RetT test3(T1, T2, T3) { return RetT(); }
template <typename T1, typename T2, typename T3> static inline void test3(T1, T2, T3) {}


inline FunctionGroup::Ptr axtestscalar()
{
    return FunctionBuilder("TestScalarFunction")
        .addSignature<void(double)>((void(*)(double))(test1), "tsfd")
        .addSignature<void(float)>((void(*)(float))(test1), "tsff")
        .addSignature<void(int64_t)>((void(*)(int64_t))(test1), "tsfi64")
        .addSignature<void(int32_t)>((void(*)(int32_t))(test1), "tsfi32")
        .addSignature<void(int16_t)>((void(*)(int16_t))(test1), "tsfi16")
        .addSignature<void(bool)>((void(*)(bool))(test1), "tsfb")
        .get();
}

inline FunctionGroup::Ptr axtestsize()
{
    return FunctionBuilder("TestSizeFunction")
        .addSignature<void()>((void(*)())(test), "tsf")
        .addSignature<void(double)>((void(*)(double))(test1), "tsfd")
        .addSignature<void(double,double)>((void(*)(double,double))(test2), "tsfdd")
        .get();
}

inline FunctionGroup::Ptr axtestmulti()
{
    /// @todo  There are a bunch of invalid methods that conflict in this definition.
    ///        The function framework needs to be updated to catch these:
    /// 1) tmf and tmfvi2r rebuild to the same arg signature
    /// 2) tmfdd and tmfiid are ambiguous on implicit match
    return FunctionBuilder("TestMultiFunction")
        .addSignature<void()>((void(*)())(test), "tmf")
        .addSignature<void(int32_t)>((void(*)(int32_t))(test1), "tmfi")
        .addSignature<void(double,double)>((void(*)(double,double))(test2), "tmfdd")
        .addSignature<int32_t(int32_t,double)>((int32_t(*)(int32_t,double))(test2), "tmfiid")
        .addSignature<void(double*,int32_t,double), true>((void(*)(double*,int32_t,double))(test3), "tmfidd")
        .addSignature<void(int32_t(*)[1])>((void(*)(int32_t(*)[1]))(test1), "tmfvi1")
        .addSignature<void(int32_t(*)[2])>((void(*)(int32_t(*)[2]))(test1), "tmfvi2")
        .addSignature<void(int32_t(*)[2]), true>((void(*)(int32_t(*)[2]))(test1), "tmfvi2r")
        .get();
}

inline FunctionGroup::Ptr axirfunction()
{
    static auto generate =
        [](const std::vector<llvm::Value*>& args,
           const std::unordered_map<std::string, llvm::Value*>&,
           llvm::IRBuilder<>& B) -> llvm::Value*
    {
        if (args.empty())     return nullptr;
        if (args.size() != 1) return nullptr;
        if (args[0]->getType()->isDoubleTy())     return llvm::ConstantFP::get(LLVMType<double>::get(B.getContext()), 1.0);
        if (args[0]->getType()->isFloatTy())      return llvm::ConstantFP::get(LLVMType<float>::get(B.getContext()), 1.0f);
        if (args[0]->getType()->isIntegerTy(32))  return B.getInt32(1);
        if (args[0]->getType()->isIntegerTy(8))   return B.getInt32(1); // unexpected return type for int8
        return nullptr;
    };

    return FunctionBuilder("TestIRFunction")
        .addSignature<double(double)>(generate, "double")
        .addSignature<float(float)>(generate, "float")
        .addSignature<int32_t(int32_t)>(generate, "int32")
        .addSignature<void(int8_t)>(generate, "int8")
        .get();
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

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

    FunctionGroup::Ptr group = axtestscalar();

    CPPUNIT_ASSERT_EQUAL(std::string("TestScalarFunction"), std::string(group->name()));

    const std::vector<Function::Ptr>* list = &group->list();
    CPPUNIT_ASSERT_EQUAL(size_t(6), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("tsfd"), std::string((*list)[0]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsff"), std::string((*list)[1]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi64"), std::string((*list)[2]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi32"), std::string((*list)[3]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfi16"), std::string((*list)[4]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfb"), std::string((*list)[5]->symbol()));

    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[3]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[4]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[5]->size());

    std::vector<const CFunctionBase*> bindings;
    bindings.reserve(6);
    for (auto& decl : *list) {
        bindings.emplace_back(dynamic_cast<const CFunctionBase*>(decl.get()));
    }

    CPPUNIT_ASSERT(bindings[0]);
    CPPUNIT_ASSERT(bindings[1]);
    CPPUNIT_ASSERT(bindings[2]);
    CPPUNIT_ASSERT(bindings[3]);
    CPPUNIT_ASSERT(bindings[4]);
    CPPUNIT_ASSERT(bindings[5]);
    CPPUNIT_ASSERT(bindings[0]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[1]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[2]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[3]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[4]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[5]->address() != uint64_t(0));
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<double>), bindings[0]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<float>), bindings[1]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<int64_t>), bindings[2]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<int32_t>), bindings[3]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<int16_t>), bindings[4]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<bool>), bindings[5]->address());

    //

    group = axtestsize();
    CPPUNIT_ASSERT_EQUAL(std::string("TestSizeFunction"), std::string(group->name()));

    list = &group->list();
    CPPUNIT_ASSERT_EQUAL(size_t(3), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("tsf"), std::string((*list)[0]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfd"), std::string((*list)[1]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tsfdd"), std::string((*list)[2]->symbol()));

    CPPUNIT_ASSERT_EQUAL(size_t(0), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[2]->size());

    bindings.clear();
    bindings.reserve(3);
    for (auto& decl : *list) {
        bindings.emplace_back(dynamic_cast<const CFunctionBase*>(decl.get()));
    }

    CPPUNIT_ASSERT(bindings[0]);
    CPPUNIT_ASSERT(bindings[1]);
    CPPUNIT_ASSERT(bindings[2]);
    CPPUNIT_ASSERT(bindings[0]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[1]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[2]->address() != uint64_t(0));
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test), bindings[0]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test1<double>), bindings[1]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>(&test2<double, double>), bindings[2]->address());

    //

    group = axtestmulti();

    CPPUNIT_ASSERT_EQUAL(std::string("TestMultiFunction"), std::string(group->name()));

    list = &group->list();
    CPPUNIT_ASSERT_EQUAL(size_t(8), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("tmf"), std::string((*list)[0]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfi"), std::string((*list)[1]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfdd"), std::string((*list)[2]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfiid"), std::string((*list)[3]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfidd"), std::string((*list)[4]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfvi1"), std::string((*list)[5]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfvi2"), std::string((*list)[6]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("tmfvi2r"), std::string((*list)[7]->symbol()));

    CPPUNIT_ASSERT_EQUAL(size_t(0), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(2), (*list)[3]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(3), (*list)[4]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[5]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[6]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[7]->size());

    bindings.clear();
    bindings.reserve(8);
    for (auto& decl : *list) {
        bindings.emplace_back(dynamic_cast<const CFunctionBase*>(decl.get()));
    }

    CPPUNIT_ASSERT(bindings[0]);
    CPPUNIT_ASSERT(bindings[1]);
    CPPUNIT_ASSERT(bindings[2]);
    CPPUNIT_ASSERT(bindings[3]);
    CPPUNIT_ASSERT(bindings[4]);
    CPPUNIT_ASSERT(bindings[5]);
    CPPUNIT_ASSERT(bindings[6]);
    CPPUNIT_ASSERT(bindings[7]);
    CPPUNIT_ASSERT(bindings[0]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[1]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[2]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[3]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[4]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[5]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[6]->address() != uint64_t(0));
    CPPUNIT_ASSERT(bindings[7]->address() != uint64_t(0));
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)())(test)), bindings[0]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(int32_t))(test1)), bindings[1]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(double,double))(test2)), bindings[2]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((int32_t(*)(int32_t,double))(test2)), bindings[3]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(double*,int32_t,double))(test3)), bindings[4]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(int32_t(*)[1]))(test1)), bindings[5]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(int32_t(*)[2]))(test1)), bindings[6]->address());
    CPPUNIT_ASSERT_EQUAL(reinterpret_cast<uint64_t>((void(*)(int32_t(*)[2]))(test1)), bindings[7]->address());

    //

    group = axirfunction();

    CPPUNIT_ASSERT_EQUAL(std::string("TestIRFunction"), std::string(group->name()));

    list = &group->list();
    CPPUNIT_ASSERT_EQUAL(size_t(4), list->size());
    CPPUNIT_ASSERT_EQUAL(std::string("double"), std::string((*list)[0]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("float"), std::string((*list)[1]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("int32"), std::string((*list)[2]->symbol()));
    CPPUNIT_ASSERT_EQUAL(std::string("int8"), std::string((*list)[3]->symbol()));

    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[0]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[1]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[2]->size());
    CPPUNIT_ASSERT_EQUAL(size_t(1), (*list)[3]->size());

    std::vector<const IRFunctionBase*> irfunctions;
    irfunctions.reserve(4);
    for (auto& decl : *list) {
        irfunctions.emplace_back(dynamic_cast<const IRFunctionBase*>(decl.get()));
    }

    CPPUNIT_ASSERT(irfunctions[0]);
    CPPUNIT_ASSERT(irfunctions[1]);
    CPPUNIT_ASSERT(irfunctions[2]);
    CPPUNIT_ASSERT(irfunctions[3]);
}

void
TestFunctionBase::testMatching()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionGroup::Ptr group = axtestscalar();

    const std::vector<Function::Ptr>* list = &group->list();

    std::vector<llvm::Type*> types;
    Function::SignatureMatch match;
    Function::Ptr result;

    types.resize(1);

    // test explicit matching

    types[0] = LLVMType<bool>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[5], result);
    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[5], result);

    //

    types[0] = LLVMType<int16_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[4], result);

    //

    types[0] = LLVMType<int32_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[3], result);

    //

    types[0] = LLVMType<int64_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    //

    types[0] = LLVMType<float>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[1], result);

    //

    types[0] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[0], result);

    //

    types[0] = LLVMType<int64_t>::get(state.context()); // int64_t

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    // test unsigned integers automatic type creation - these are not supported in the
    // language however can be constructed from the API. The function framework does
    // not differentiate between signed and unsigned integers

    types[0] = LLVMType<uint64_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    // test implicit matching - types should match to the first available castable signature
    // which is always the void(double) "tsfd" function for all provided scalars

    types[0] = LLVMType<int8_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Implicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[0], result);

    types.clear();

    // test invalid matching - Size matching returns the first function which matched
    // the size

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT_EQUAL(Function::SignatureMatch::None, match);
    CPPUNIT_ASSERT(!result);
    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::None == match);
    CPPUNIT_ASSERT(!result);

    //

    types.emplace_back(LLVMType<bool*>::get(state.context()));

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Size == match);
    CPPUNIT_ASSERT(!result);

    //

    types[0] = LLVMType<bool[1]>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Size == match);
    CPPUNIT_ASSERT(!result);

    //

    types[0] = LLVMType<bool>::get(state.context());
    types.emplace_back(LLVMType<bool>::get(state.context()));

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::None == match);
    CPPUNIT_ASSERT(!result);

    // Test varying argument size function

    // test explicit matching

    group = axtestsize();
    list = &group->list();

    types.resize(2);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    //

    types.resize(1);

    types[0] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[1], result);

    //

    types.clear();

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[0], result);

    // Test implicit matching

    types.resize(2);

    types[0] = LLVMType<float>::get(state.context());
    types[1] = LLVMType<int32_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Implicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    // Test non matching

    types.resize(3);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());
    types[2] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::None == match);
    CPPUNIT_ASSERT(!result);

    // Test multi function

    group = axtestmulti();
    list = &group->list();

    // test explicit/implicit matching

    types.clear();

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[0], result);

    //

    types.resize(2);

    types[0] = LLVMType<double>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    //

    types[0] = LLVMType<int32_t>::get(state.context());
    types[1] = LLVMType<double>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[3], result);

    //

    types[0] = LLVMType<int32_t>::get(state.context());
    types[1] = LLVMType<int32_t>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Implicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[2], result);

    //

    types.resize(1);

    types[0] = LLVMType<int32_t(*)[1]>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[5], result);

    //

    types[0] = LLVMType<int32_t(*)[2]>::get(state.context());

    result = group->match(types, state.context(), &match);
    CPPUNIT_ASSERT(Function::SignatureMatch::Explicit == match);
    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT_EQUAL((*list)[6], result);
}

void
TestFunctionBase::testExecuteCall()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionGroup::Ptr group = axtestscalar();

    const std::vector<Function::Ptr>* list = &group->list();

    // test execution

    llvm::Value* result = nullptr;
    llvm::CallInst* call = nullptr;
    llvm::Function* target = nullptr;
    std::vector<llvm::Value*> args;

    // test invalid arguments throws

    CPPUNIT_ASSERT_THROW(group->execute(/*args*/{}, /*globals*/{}, builder),
        openvdb::LLVMFunctionError);

    args.resize(1);

    // test llvm function calls - execute and get the called function. check this is already
    // inserted into the module and is expected llvm::Function using create on the
    // expected function signature

    args[0] = builder.getTrue();
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[5]->create(state.module()), target);

    //

    args[0] = builder.getInt16(1);
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[4]->create(state.module()), target);

    //

    args[0] = builder.getInt32(1);
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[3]->create(state.module()), target);

    //

    args[0] = builder.getInt64(1);
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[2]->create(state.module()), target);

    //

    args[0] = llvm::ConstantFP::get(LLVMType<float>::get(state.context()), 1.0f);
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[1]->create(state.module()), target);

    //

    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);
    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[0]->create(state.module()), target);


    // Test multi function

    group = axtestmulti();

    list = &group->list();
    args.clear();

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[0]->create(state.module()), target);

    //

    args.resize(1);

    args[0] = builder.CreateAlloca(llvm::ArrayType::get(LLVMType<int32_t>::get(state.context()), 1));

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[5]->create(state.module()), target);

    //

    args[0] = builder.CreateAlloca(llvm::ArrayType::get(LLVMType<int32_t>::get(state.context()), 2));

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[6]->create(state.module()), target);

    //

    args.resize(2);

    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);
    args[1] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    call = llvm::cast<llvm::CallInst>(result);

    CPPUNIT_ASSERT(call);
    target = call->getCalledFunction();
    CPPUNIT_ASSERT(target);
    CPPUNIT_ASSERT_EQUAL((*list)[2]->create(state.module()), target);
}

void
TestFunctionBase::testGenerate()
{
    unittest_util::LLVMState state;
    llvm::IRBuilder<> builder(state.scratchBlock());

    FunctionGroup::Ptr group = axirfunction();

    llvm::Value* result = nullptr;
    std::vector<llvm::Value*> args;

    // test invalid arguments throws

    CPPUNIT_ASSERT_THROW(group->execute(/*args*/{}, /*globals*/{}, builder),
        openvdb::LLVMFunctionError);

    // test IR generation

    args.resize(1);
    args[0] = llvm::ConstantFP::get(LLVMType<double>::get(state.context()), 1.0);

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), result->getType());

    //

    args[0] = llvm::ConstantFP::get(LLVMType<float>::get(state.context()), 1.0);

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<float>::get(state.context()), result->getType());

    //

    args[0] = builder.getInt32(1);

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<int32_t>::get(state.context()), result->getType());

    // test values are cast with implicit matching

    args[0] = builder.getInt16(1);

    result = group->execute(args, {}, builder);

    CPPUNIT_ASSERT(result);
    CPPUNIT_ASSERT(llvm::isa<llvm::CallInst>(result));
    CPPUNIT_ASSERT_EQUAL(LLVMType<double>::get(state.context()), result->getType());

    // test throw on unexpected return type

    args[0] = builder.getInt8(1);

    CPPUNIT_ASSERT_THROW(group->execute(/*args*/{}, {}, builder),
        openvdb::LLVMFunctionError);
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
