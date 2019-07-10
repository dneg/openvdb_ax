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

#include "TestHarness.h"

#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

using namespace openvdb::points;

class TestCast : public unittest_util::AXTestCase
{
public:

    CPPUNIT_TEST_SUITE(TestCast);
    CPPUNIT_TEST(castDouble);
    CPPUNIT_TEST(castFloat);
    CPPUNIT_TEST(castInt);
    CPPUNIT_TEST(castLong);
    CPPUNIT_TEST(castShort);
    CPPUNIT_TEST(castLiteral);
    CPPUNIT_TEST(castImplicitAssign);
    CPPUNIT_TEST(castImplicitArithmetic);
    CPPUNIT_TEST(castImplicitComponentAssign);

    // the following are atlernative versions of the preceding cast tests but which do not
    // use "short" data since we don't currently support "short" data volumes.  If this is fixed,
    // these tests should be deprecated.
    CPPUNIT_TEST(castDoubleVolume);
    CPPUNIT_TEST(castFloatVolume);
    CPPUNIT_TEST(castIntVolume);
    CPPUNIT_TEST(castLongVolume);


    CPPUNIT_TEST_SUITE_END();

    void castDouble();
    void castFloat();
    void castInt();
    void castLong();
    void castShort();
    void castLiteral();
    void castImplicitAssign();
    void castImplicitArithmetic();
    void castImplicitComponentAssign();
    void castDoubleVolume();
    void castFloatVolume();
    void castIntVolume();
    void castLongVolume();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestCast);

void
TestCast::castDouble()
{
    mHarness.testVolumes(false);
    mHarness.addAttribute<short>("short_test", short(1));
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));
    mHarness.addAttribute<double>("double_test2", double(1.0));

    mHarness.executeCode("test/snippets/cast/castDouble");

    AXTESTS_STANDARD_ASSERT()
}

void
TestCast::castFloat()
{
    mHarness.testVolumes(false);
    mHarness.addAttribute<short>("short_test", short(1));
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<float>("float_test2", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castFloat");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castInt()
{
    mHarness.testVolumes(false);
    mHarness.addAttribute<short>("short_test", short(1));
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int>("int_test2", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castInt");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castLong()
{
    mHarness.testVolumes(false);
    mHarness.addAttribute<short>("short_test", short(1));
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<int64_t>("long_test2", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castLong");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castShort()
{
    mHarness.testVolumes(false);
    mHarness.addAttribute<short>("short_test", short(1));
    mHarness.addAttribute<short>("short_test2", short(1));
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castShort");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castLiteral()
{
    mHarness.addAttribute<float>("float_test", float(10.0));
    mHarness.executeCode("test/snippets/cast/castLiteral");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castImplicitAssign()
{
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 3), {10.0f, 0.0f, 5.2f});
    mHarness.addAttributes<openvdb::Vec3f>({"vec_float_test1", "vec_float_test2"},
        {openvdb::Vec3f(1.0f, 1.0f, 1.0f), openvdb::Vec3f(openvdb::Vec3d(1.0, 3.4, 6.0))});
    mHarness.addAttribute("vec_double_test1", openvdb::Vec3d(openvdb::Vec3i(3, 5, -6)));
    mHarness.addAttributes<int>(unittest_util::nameSequence("int_test", 3), {0, 0, 5});
    mHarness.addAttribute<int64_t>("long_test1", 2147483648l);

    mHarness.executeCode("test/snippets/cast/castImplicitAssign");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castImplicitArithmetic()
{
    mHarness.addAttribute<float>("float_test1", 2.5f);
    mHarness.addAttribute<float>("float_test2", 2.5f + (5 - 4.2f));
    mHarness.addAttribute<openvdb::Vec3f>("vec_float_test1", openvdb::Vec3f(5 - 4.2f));
    mHarness.addAttribute<openvdb::Vec3i>("vec_int_test1", openvdb::Vec3i(10));
    mHarness.addAttribute<openvdb::Vec3i>("vec_int_test2", openvdb::Vec3i(-1));
    mHarness.addAttribute<bool>("bool_test", true);

    mHarness.executeCode("test/snippets/cast/castImplicitArithmetic");

   AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castImplicitComponentAssign()
{
    mHarness.addAttribute("vec_float_test1", openvdb::Vec3f(-5.361f, 10.3f, 6.0f));
    mHarness.addAttribute("vec_float_test2", openvdb::Vec3f(40, 40, 40));
    mHarness.addAttribute<int>("int_test1", -5);
    mHarness.addAttribute<int64_t>("long_test1", 40);

    mHarness.executeCode("test/snippets/cast/castImplicitComponentAssign");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castDoubleVolume()
{
    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));
    mHarness.addAttribute<double>("double_test2", double(1.0));

    mHarness.executeCode("test/snippets/cast/castDoubleVolume");

    AXTESTS_STANDARD_ASSERT()
}


void
TestCast::castFloatVolume()
{
    mHarness.testPoints(false);

    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<float>("float_test2", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castFloatVolume");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castIntVolume()
{
    mHarness.testPoints(false);

    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int>("int_test2", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castIntVolume");

    AXTESTS_STANDARD_ASSERT();
}

void
TestCast::castLongVolume()
{
    mHarness.testPoints(false);

    mHarness.addAttribute<int>("int_test", int(1));
    mHarness.addAttribute<int64_t>("long_test", int64_t(1));
    mHarness.addAttribute<int64_t>("long_test2", int64_t(1));
    mHarness.addAttribute<float>("float_test", float(1.0f));
    mHarness.addAttribute<double>("double_test", double(1.0));

    mHarness.executeCode("test/snippets/cast/castLongVolume");

    AXTESTS_STANDARD_ASSERT();
}


// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
