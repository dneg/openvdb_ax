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


#ifndef OPENVDB_AX_UNITTEST_UTIL_HAS_BEEN_INCLUDED
#define OPENVDB_AX_UNITTEST_UTIL_HAS_BEEN_INCLUDED

#include <openvdb/Types.h>

#include <memory>
#include <vector>
#include <utility>
#include <string>

#define ERROR_MSG(Msg, Code) Msg + std::string(":\"") + Code + std::string("\"")

#define TEST_SYNTAX(Tests) \
{ \
    for (const auto& test : Tests) { \
        const unittest_util::ExpectedBase::Ptr behaviour = test.second; \
        const std::string& code = test.first; \
        if (behaviour->passes()) { \
            CPPUNIT_ASSERT_NO_THROW_MESSAGE(ERROR_MSG("Unexpected parsing error", code), \
                openvdb::ax::ast::parse(code.c_str())); \
        } \
        else { \
            CPPUNIT_ASSERT_THROW_MESSAGE(ERROR_MSG("Expected LLVMSyntaxError", code), \
                openvdb::ax::ast::parse(code.c_str()), openvdb::LLVMSyntaxError); \
        } \
    } \
} \

namespace unittest_util
{

struct ExpectedBase
{
    enum BehaviourFlags
    {
        Pass = 1 << 0
    };

    using Ptr = std::shared_ptr<ExpectedBase>;

    template <typename T>
    bool isType() const { return this->type() == openvdb::typeNameAsString<T>(); }
    virtual std::string type() const = 0;

    inline bool passes() const { return hasFlag(Pass); }
    inline bool fails() const { return !passes(); }
    inline bool hasFlag(const size_t flag) const { return mFlags & flag; }
    inline size_t count() const { return mCount; }

protected:
    ExpectedBase(const size_t flags, const size_t count)
        : mFlags(flags), mCount(count) {}

    const size_t mFlags; // expected behaviour flags per test
    const size_t mCount; // expected ast visit count
};

template <typename Type=void>
struct ExpectedType : public ExpectedBase
{
    ExpectedType(const size_t flags = 0, const size_t count = 1)
        : ExpectedBase(flags, count) {}
    std::string type() const override final { return openvdb::typeNameAsString<Type>(); }
};

using CodeTests = std::vector<std::pair<std::string, ExpectedBase::Ptr>>;


inline std::vector<std::string>
nameSequence(const std::string& base, const int number)
{
    std::vector<std::string> names;
    if (number <= 0) return names;
    names.reserve(number);

    for (int i = 1; i <= number; i++) {
        names.emplace_back(base + std::to_string(i));
    }

    return names;
}

}

#endif // OPENVDB_AX_UNITTEST_UTIL_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
