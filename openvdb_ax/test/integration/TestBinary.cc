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

class TestBinary : public unittest_util::AXTestCase
{
public:

    CPPUNIT_TEST_SUITE(TestBinary);
    CPPUNIT_TEST(testBitwise);
    CPPUNIT_TEST(testFloatingArithmetic);
    CPPUNIT_TEST(testIntegerArithmetic);
    CPPUNIT_TEST(testLogical);
    CPPUNIT_TEST(testRelational);
    CPPUNIT_TEST(testVectorArithmetic);
    CPPUNIT_TEST(testVectorRelational);
    CPPUNIT_TEST_SUITE_END();

    void testBitwise();
    void testFloatingArithmetic();
    void testIntegerArithmetic();
    void testLogical();
    void testRelational();
    void testVectorArithmetic();
    void testVectorRelational();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestBinary);

void
TestBinary::testBitwise()
{
    mHarness.addAttributes<int>(unittest_util::nameSequence("int_test", 10), {1, 7, 6, 3, -5, -8, -8, 40, 0, -1});
    mHarness.executeCode("test/snippets/binary/binaryBitwise");

    AXTESTS_STANDARD_ASSERT()
}

void
TestBinary::testFloatingArithmetic()
{
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 4),
        {4.5f, 5.0f, -0.5f, 0.8f});
    mHarness.executeCode("test/snippets/binary/binaryFloatingArithmetic");

    AXTESTS_STANDARD_ASSERT();

}

void
TestBinary::testIntegerArithmetic()
{
    mHarness.addAttributes<int>(unittest_util::nameSequence("int_test", 8),
        {3, 4, 1, 1, -1, 2, 2, -2});
    mHarness.executeCode("test/snippets/binary/binaryIntegerArithmetic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestBinary::testLogical()
{
    mHarness.addAttributes<bool>(unittest_util::nameSequence("bool_test", 6),
        {true, false, false, true, true, false});
    mHarness.executeCode("test/snippets/binary/binaryLogical");

    AXTESTS_STANDARD_ASSERT();
}

void
TestBinary::testRelational()
{
    std::vector<bool> attrValues{true, false, true, false, true, false, false, true, false, true, true,
                                false, true, false, true, false, false, true, false, true};
    mHarness.addAttributes(unittest_util::nameSequence("bool_test", 20), attrValues);
    mHarness.executeCode("test/snippets/binary/binaryRelational");

    AXTESTS_STANDARD_ASSERT();
}

void
TestBinary::testVectorArithmetic()
{
    std::vector<openvdb::Vec3f> vecfAttrValues{openvdb::Vec3f(4.f, 1.f, 5.f), openvdb::Vec3f(-2.f, 3.f, 1.f),
                                               openvdb::Vec3f(2.f, 4.f, 6.f), openvdb::Vec3f(0.5f, 1.0f, 1.5f)};
    std::vector<openvdb::Vec3i> veciAttrValues{openvdb::Vec3i(4, 1, 5), openvdb::Vec3i(-2, 3, 1),
                                               openvdb::Vec3i(2, 4, 6), openvdb::Vec3i(0, 1, 1)};

    mHarness.addAttributes(unittest_util::nameSequence("vec_float_test", 4), vecfAttrValues);
    mHarness.addAttributes(unittest_util::nameSequence("vec_int_test", 4), veciAttrValues);

    mHarness.executeCode("test/snippets/binary/binaryVectorArithmetic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestBinary::testVectorRelational()
{
    std::vector<bool> attrValues{true, false, false, false, false, false, true, true, true, true,
                                 true, false, false, false, false, false, true, true, true, true};
    mHarness.addAttributes(unittest_util::nameSequence("bool_test", 20), attrValues);
    mHarness.executeCode("test/snippets/binary/binaryVectorRelational");

    AXTESTS_STANDARD_ASSERT();
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
