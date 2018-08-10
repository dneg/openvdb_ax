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

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/Exceptions.h>
#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

#include <string>

namespace {

enum BehaviourFlags
{
    Pass = unittest_util::ExpectedBase::Pass,  // else Fails
    MaxOverflow = int16_t(Pass) << 1,
};

#define EXPECTED_PASS(Type, Flags, Count) \
    unittest_util::ExpectedBase::Ptr(new unittest_util::ExpectedType<Type>(BehaviourFlags::Pass | Flags, Count))

static const unittest_util::CodeTests tests =
{
    { ".123456789;",                EXPECTED_PASS(double, 0, 1) },
    { ".123456789f;",               EXPECTED_PASS(float, 0, 1) },
    { "0.0;",                       EXPECTED_PASS(double, 0, 1) },
    { "0.0f;",                      EXPECTED_PASS(float, 0, 1) },
    { "00.f;",                      EXPECTED_PASS(float, 0, 1) },
    { "00;",                        EXPECTED_PASS(int32_t, 0, 1) },
    { "00s;",                       EXPECTED_PASS(int16_t, 0, 1) },
    { "01l;",                       EXPECTED_PASS(int64_t, 0, 1) },
    { "0;",                         EXPECTED_PASS(int32_t, 0, 1) },
    { "0e+0f;",                     EXPECTED_PASS(float, 0, 1) },
    { "0e-0f;",                     EXPECTED_PASS(float, 0, 1) },
    { "0e0;",                       EXPECTED_PASS(double, 0, 1) },
    { "0e0f;",                      EXPECTED_PASS(float, 0, 1) },
    { "0l;",                        EXPECTED_PASS(int64_t, 0, 1) },
    { "0s;",                        EXPECTED_PASS(int16_t, 0, 1) },
    { "1.0;",                       EXPECTED_PASS(double, 0, 1) },
    { "1000000000000000;",          EXPECTED_PASS(int32_t, 0, 1) }, // signed int wrap
    { "1234567890.00000000;",       EXPECTED_PASS(double, 0, 1) },
    { "1234567890.0987654321;",     EXPECTED_PASS(double, 0, 1) },
    { "1234567890.0987654321f;",    EXPECTED_PASS(float, 0, 1) },
    { "1234567890.10000000;",       EXPECTED_PASS(double, 0, 1) },
    { "1234567890;",                EXPECTED_PASS(int32_t, 0, 1) },
    { "1234567890e-0;",             EXPECTED_PASS(double, 0, 1) },
    { "1234567890l;",               EXPECTED_PASS(int64_t, 0, 1) },
    { "1234567890s;",               EXPECTED_PASS(int16_t, 0, 1) }, // signed int wrap
    { "1;",                         EXPECTED_PASS(int32_t, 0, 1) },
    { "1e+6;",                      EXPECTED_PASS(double, 0, 1) },
    { "1e+6f;",                     EXPECTED_PASS(float, 0, 1) },
    { "1E+6f;",                     EXPECTED_PASS(float, 0, 1) },
    { "1e-6;",                      EXPECTED_PASS(double, 0, 1) },
    { "1e-6f;",                     EXPECTED_PASS(float, 0, 1) },
    { "1E-6f;",                     EXPECTED_PASS(float, 0, 1) },
    { "1e01;",                      EXPECTED_PASS(double, 0, 1) },
    { "1e123456789;",               EXPECTED_PASS(double, BehaviourFlags::MaxOverflow, 1) },
    { "1e123456789f;",              EXPECTED_PASS(float, BehaviourFlags::MaxOverflow, 1) },
    { "1e6;",                       EXPECTED_PASS(double, 0, 1) },
    { "1E6;",                       EXPECTED_PASS(double, 0, 1) },
    { "1e6f;",                      EXPECTED_PASS(float, 0, 1) },
    { "1E6f;",                      EXPECTED_PASS(float, 0, 1) },
    { "\"0.0\";",                   EXPECTED_PASS(std::string, 0, 1) },
    { "\"0.0f\";",                  EXPECTED_PASS(std::string, 0, 1) },
    { "\"0\";",                     EXPECTED_PASS(std::string, 0, 1) },
    { "\"1234567890.0987654321\";", EXPECTED_PASS(std::string, 0, 1) },
    { "\"1234567890\";",            EXPECTED_PASS(std::string, 0, 1) },
    { "\"a1b2c3d4.e5f6g7.0\";",     EXPECTED_PASS(std::string, 0, 1) },
    { "\"literal\";",               EXPECTED_PASS(std::string, 0, 1) },
    { "\"\";",                      EXPECTED_PASS(std::string, 0, 1) },
    { "false;",                     EXPECTED_PASS(bool, 0, 1) },
    { "true;",                      EXPECTED_PASS(bool, 0, 1) },

    { "\"" + std::to_string(std::numeric_limits<double>::lowest()) + "\";",  EXPECTED_PASS(std::string, 0, 1) },
    { "\"" + std::to_string(std::numeric_limits<double>::max()) + "\";",     EXPECTED_PASS(std::string, 0, 1) },
    { "\"" + std::to_string(std::numeric_limits<int64_t>::lowest()) + "\";", EXPECTED_PASS(std::string, 0, 1) },
    { "\"" + std::to_string(std::numeric_limits<int64_t>::max()) + "\";",    EXPECTED_PASS(std::string, 0, 1) },
    // No limits::lowest, negative values are a unary operator
    { std::to_string(std::numeric_limits<double>::max()) + ";",    EXPECTED_PASS(double, 0, 1) },
    { std::to_string(std::numeric_limits<double>::min()) + ";",    EXPECTED_PASS(double, 0, 1) },
    { std::to_string(std::numeric_limits<int16_t>::max()) + "0s;", EXPECTED_PASS(int16_t, 0, 1) }, // signed int wrap
    { std::to_string(std::numeric_limits<int32_t>::max()) + "0;",  EXPECTED_PASS(int32_t, 0, 1) }, // signed int wrap
    { std::to_string(std::numeric_limits<int64_t>::max()) + "0l;", EXPECTED_PASS(int64_t, BehaviourFlags::MaxOverflow, 1) },
    { std::to_string(std::numeric_limits<int64_t>::max()) + ";",   EXPECTED_PASS(int32_t, 0, 1) }, // signed int wrap
    { std::to_string(std::numeric_limits<uint64_t>::max()) + "0;", EXPECTED_PASS(int32_t, BehaviourFlags::MaxOverflow, 1) },
    { std::to_string(std::numeric_limits<uint64_t>::max()) + ";",  EXPECTED_PASS(int32_t, 0, 1) }, // signed int wrap
    { std::to_string(std::numeric_limits<uint64_t>::max()) + "s;", EXPECTED_PASS(int16_t, 0, 1) }, // signed int wrap
    { std::to_string(uint64_t(std::numeric_limits<int64_t>::max()) + 1) + "l;",  EXPECTED_PASS(int64_t, 0, 1) }, // signed int wrap
};

struct ValueVisitor : public openvdb::ax::ast::Visitor
{
    ValueVisitor()
        : mCount(0), mNode(nullptr) {}
    ~ValueVisitor() override = default;

    inline virtual void
    visit(const openvdb::ax::ast::Value<bool>& node) override final {
        visitValueBase(&node);
    };

    inline virtual void
    visit(const openvdb::ax::ast::Value<std::string>& node) override final {
        visitValueBase(&node);
    }

    inline virtual void
    visit(const openvdb::ax::ast::Value<int16_t>& node) override final {
        visitValueBase(&node);
    };

    inline virtual void
    visit(const openvdb::ax::ast::Value<int32_t>& node) override final {
        visitValueBase(&node);
    };

    inline virtual void
    visit(const openvdb::ax::ast::Value<int64_t>& node) override final {
        visitValueBase(&node);
    };

    inline virtual void
    visit(const openvdb::ax::ast::Value<float>& node) override final {
        visitValueBase(&node);
    };

    inline virtual void
    visit(const openvdb::ax::ast::Value<double>& node) override final {
        visitValueBase(&node);
    };

    size_t mCount; // tracks the amount of nodes visited
    const openvdb::ax::ast::ValueBase* mNode;

private:
    inline void visitValueBase(const openvdb::ax::ast::ValueBase* node) {
        ++mCount;
        mNode = node;
    }
};

inline std::string
stripToValue(const std::string& string)
{
    std::string stripped = string;
    stripped.erase(std::remove(stripped.begin(), stripped.end(), '"'), stripped.end());
    stripped.erase(std::remove(stripped.begin(), stripped.end(), ';'), stripped.end());
    return stripped;
}

}

class TestValueNode : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestValueNode);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST(testASTNode);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax() { TEST_SYNTAX(tests); }
    void testASTNode();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestValueNode);

void
TestValueNode::testASTNode()
{
    using namespace openvdb::ax::ast;

    for (const auto& test : tests) {
        const unittest_util::ExpectedBase::Ptr behaviour = test.second;
        if (!behaviour->passes()) continue;

        const std::string& code = test.first;
        const openvdb::ax::ast::Tree::Ptr tree = openvdb::ax::ast::parse(code.c_str());
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No AST returned", code), static_cast<bool>(tree));

        ValueVisitor visitor;
        tree->accept(visitor);

        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid AST node count", code),
            size_t(1), visitor.mCount);

        const ValueBase* valueBase = visitor.mNode;
        const std::string stringValue = stripToValue(code);

        if (behaviour->isType<bool>()) {
            // should be impossible for bools to overflow
            CPPUNIT_ASSERT(!behaviour->hasFlag(BehaviourFlags::MaxOverflow));

            const Value<bool>* value =
                dynamic_cast<const Value<bool>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<bool> type found", code), value);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Bool value has overflowed", code), !value->mText);

            const bool isTrue = stringValue.find("true") != std::string::npos;
            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected bool value is invalid", code),
                bool(value->mValue), isTrue);
        }
        else if (behaviour->isType<std::string>()) {
            // Should be impossible for strings to overflow
            CPPUNIT_ASSERT(!behaviour->hasFlag(BehaviourFlags::MaxOverflow));

            const Value<std::string>* value =
                dynamic_cast<const Value<std::string>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<std::string> type found", code), value);

            CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Expected string value is invalid", code),
                value->mValue, stringValue);
        }
        else if (behaviour->isType<int16_t>()) {
            const Value<int16_t>* value =
                dynamic_cast<const Value<int16_t>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<int16_t> type found", code), value);

            if (behaviour->hasFlag(BehaviourFlags::MaxOverflow)) {
                CPPUNIT_ASSERT(value->mText);
                CPPUNIT_ASSERT_EQUAL(*(value->mText), stringValue);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected overflow behaviour", code),
                    openvdb::ax::LiteralLimits<int16_t>::onLimitOverflow(), value->mValue);
            }
            else {
                CPPUNIT_ASSERT(!value->mText);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Corresponding literal value is invalid", code),
                    openvdb::ax::LiteralLimits<int16_t>::convert(stringValue), value->mValue);
            }
        }
        else if (behaviour->isType<int32_t>()) {
            const Value<int32_t>* value =
                dynamic_cast<const Value<int32_t>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<int32_t> type found", code), value);

            if (behaviour->hasFlag(BehaviourFlags::MaxOverflow)) {
                CPPUNIT_ASSERT(value->mText);
                CPPUNIT_ASSERT_EQUAL(*(value->mText), stringValue);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected overflow behaviour", code),
                    openvdb::ax::LiteralLimits<int32_t>::onLimitOverflow(), value->mValue);
            }
            else {
                CPPUNIT_ASSERT(!value->mText);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Corresponding literal value is invalid", code),
                    openvdb::ax::LiteralLimits<int32_t>::convert(stringValue), value->mValue);
            }
        }
        else if (behaviour->isType<int64_t>()) {
            const Value<int64_t>* value =
                dynamic_cast<const Value<int64_t>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<int64_t> type found", code), value);

            if (behaviour->hasFlag(BehaviourFlags::MaxOverflow)) {
                CPPUNIT_ASSERT(value->mText);
                CPPUNIT_ASSERT_EQUAL(*(value->mText), stringValue);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected overflow behaviour", code),
                    openvdb::ax::LiteralLimits<int64_t>::onLimitOverflow(), value->mValue);
            }
            else {
                CPPUNIT_ASSERT(!value->mText);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Corresponding literal value is invalid", code),
                    openvdb::ax::LiteralLimits<int64_t>::convert(stringValue), value->mValue);
            }
        }
        else if (behaviour->isType<float>()) {
            const Value<float>* value =
                dynamic_cast<const Value<float>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<float> type found", code), value);

            if (behaviour->hasFlag(BehaviourFlags::MaxOverflow)) {
                CPPUNIT_ASSERT(value->mText);
                CPPUNIT_ASSERT_EQUAL(*(value->mText), stringValue);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected overflow behaviour", code),
                    openvdb::ax::LiteralLimits<float>::onLimitOverflow(), value->mValue);
            }
            else {
                CPPUNIT_ASSERT(!value->mText);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Corresponding literal value is invalid", code),
                    openvdb::ax::LiteralLimits<float>::convert(stringValue), value->mValue);
            }
        }
        else if (behaviour->isType<double>()) {
            const Value<double>* value =
                dynamic_cast<const Value<double>*>(valueBase);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("No ast::Value<double> type found", code), value);

            if (behaviour->hasFlag(BehaviourFlags::MaxOverflow)) {
                CPPUNIT_ASSERT(value->mText);
                CPPUNIT_ASSERT_EQUAL(*(value->mText), stringValue);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Unexpected overflow behaviour", code),
                    openvdb::ax::LiteralLimits<double>::onLimitOverflow(), value->mValue);
            }
            else {
                CPPUNIT_ASSERT(!value->mText);
                CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Corresponding literal value is invalid", code),
                    openvdb::ax::LiteralLimits<double>::convert(stringValue), value->mValue);
            }
        }
        else {
            CPPUNIT_FAIL(ERROR_MSG("Invalid expected behaviour value type for Literal Value operation", code));
        }
    }
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
