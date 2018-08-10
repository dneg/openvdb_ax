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

#include "TestHarness.h"

#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

#include <boost/functional/hash.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>

using namespace openvdb::points;

class TestFunction : public unittest_util::AXTestCase
{
public:
    CPPUNIT_TEST_SUITE(TestFunction);
    CPPUNIT_TEST(testFunctionAbs);
    CPPUNIT_TEST(testFunctionClamp);
    CPPUNIT_TEST(testFunctionDot);
    CPPUNIT_TEST(testFunctionFit);
    CPPUNIT_TEST(testFunctionImplicitArguments);
    CPPUNIT_TEST(testFunctionIntrinsic);
    CPPUNIT_TEST(testFunctionLength);
    CPPUNIT_TEST(testFunctionLengthSq);
    CPPUNIT_TEST(testFunctionCeil);
    CPPUNIT_TEST(testFunctionRound);
    CPPUNIT_TEST(testFunctionMax);
    CPPUNIT_TEST(testFunctionMin);
    CPPUNIT_TEST(testFunctionNormalize);
    CPPUNIT_TEST(testFunctionRand);
    CPPUNIT_TEST(testFunctionPow);
    CPPUNIT_TEST(testFunctionVolumeIndexCoords);
    CPPUNIT_TEST(testFunctionVolumePWS);
    CPPUNIT_TEST(testFunctionDeletePoint);
    CPPUNIT_TEST_SUITE_END();

    void testFunctionAbs();
    void testFunctionClamp();
    void testFunctionDot();
    void testFunctionFit();
    void testFunctionImplicitArguments();
    void testFunctionIntrinsic();
    void testFunctionLength();
    void testFunctionLengthSq();
    void testFunctionCeil();
    void testFunctionRound();
    void testFunctionMax();
    void testFunctionMin();
    void testFunctionNormalize();
    void testFunctionRand();
    void testFunctionPow();
    void testFunctionVolumeIndexCoords();
    void testFunctionVolumePWS();
    void testFunctionDeletePoint();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFunction);

void
TestFunction::testFunctionAbs()
{
    mHarness.addAttributes<int32_t>({"test1", "test2", "test3"}, {3, 3, 0});

    mHarness.executeCode("test/snippets/function/functionAbs");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionClamp()
{
    mHarness.addAttributes<double>(unittest_util::nameSequence("double_test", 3),
        {-1.5, 0.0, 1.5});
    mHarness.executeCode("test/snippets/function/functionClamp");

    AXTESTS_STANDARD_ASSERT()
}

void
TestFunction::testFunctionDot()
{
    mHarness.addAttribute<float>("float_test", 29.0f);
    mHarness.executeCode("test/snippets/function/functionDot");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionFit()
{
    std::vector<double> values{23.0, -23.0, -25.0, -15.0, -15.0, -18.0, -24.0, 0.0, 10.0,
        -5.0, 0.0, -1.0};
    mHarness.addAttributes<double>(unittest_util::nameSequence("double_test", 12), values);

    mHarness.executeCode("test/snippets/function/functionFit");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionImplicitArguments()
{
    mHarness.addAttributes<double>(unittest_util::nameSequence("double_test", 4),
        {-1.5, 105.0, -23.0, 1.5});
    mHarness.executeCode("test/snippets/function/functionImplicitArguments");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionIntrinsic()
{
    mHarness.addAttributes<double>(unittest_util::nameSequence("double_test", 10),
        {3.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 16.0, 10.321, 2194.0});
    mHarness.executeCode("test/snippets/function/functionIntrinsic");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionLength()
{
    mHarness.addAttribute<float>("float_test", 7.0f);
    mHarness.executeCode("test/snippets/function/functionLength");

    AXTESTS_STANDARD_ASSERT()
}

void
TestFunction::testFunctionLengthSq()
{
    mHarness.addAttribute<float>("float_test", 49.0f);
    mHarness.executeCode("test/snippets/function/functionLengthSq");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionCeil()
{
    mHarness.addAttributes<float>({"float_test1", "float_test2"}, {5.0f, -4.0f});
    mHarness.addAttribute<int>("int_test1", 3);
    mHarness.executeCode("test/snippets/function/functionCeil");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionRound()
{
    mHarness.addAttributes<float>({"float_test1", "float_test2"}, {5.0f, -4.0f});
    mHarness.addAttribute<int>("int_test1", 3);
    mHarness.executeCode("test/snippets/function/functionRound");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionMax()
{
    mHarness.addAttribute("double_test", 1.5);
    mHarness.executeCode("test/snippets/function/functionMax");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionMin()
{
    mHarness.addAttribute("double_test", -1.5);
    mHarness.executeCode("test/snippets/function/functionMin");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionNormalize()
{
    openvdb::Vec3f expectedValue(1.f, 2.f, 3.f);
    expectedValue.normalize();

    mHarness.addAttribute<openvdb::Vec3f>("vec_float_test", expectedValue);
    mHarness.executeCode("test/snippets/function/functionNormalize");

    AXTESTS_STANDARD_ASSERT();

}

namespace {

unsigned int hashToSeed(size_t hash) {
    unsigned int seed = 0;
    do {
        seed ^= (unsigned int) hash;
    } while (hash >>= sizeof(unsigned int) * 8);
    return seed;
}

}

void
TestFunction::testFunctionRand()
{
    boost::uniform_01<double> uniform_01;
    size_t hash = boost::hash<double>()(2.0);
    boost::mt19937 engine(static_cast<boost::mt19937::result_type>(hashToSeed(hash)));

    const double expected1 = uniform_01(engine);

    hash = boost::hash<double>()(3.0);
    engine.seed(static_cast<boost::mt19937::result_type>(hashToSeed(hash)));
    const double expected2 = uniform_01(engine);

    mHarness.addAttributes<double>({"rand_test_0", "rand_test_1", "rand_test_2"},
        {expected1, expected1, expected2});

    mHarness.executeCode("test/snippets/function/functionRand");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionPow()
{
    mHarness.addAttributes<float>(unittest_util::nameSequence("float_test", 5, false),
        {1.0, pow(3.0, -2.1), pow(4.7f, -4.3f), pow(4.7f, 3), 0.00032f});
    mHarness.addAttribute<int>("int_test1", pow(3, 5));

    mHarness.executeCode("test/snippets/function/functionPow");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionVolumeIndexCoords()
{
    // create 3 test grids
    std::vector<openvdb::Int32Grid::Ptr> testGrids(3);
    openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform(0.1);

    int i = 0;
    for (auto& grid : testGrids) {
        grid = openvdb::Int32Grid::create();
        grid->setTransform(transform);
        grid->setName("a" + std::to_string(i));
        openvdb::Int32Grid::Accessor accessor = grid->getAccessor();
        accessor.setValueOn(openvdb::Coord(1, 2, 3), 0);
        accessor.setValueOn(openvdb::Coord(1, 10, 3), 0);
        accessor.setValueOn(openvdb::Coord(-1, 1, 10), 0);
        ++i;
    }

    // convert to GridBase::Ptr to call wrapExecution
    openvdb::GridPtrVec testGridsBase(3);

    std::copy(testGrids.begin(), testGrids.end(), testGridsBase.begin());

    unittest_util::wrapExecution(testGridsBase, "test/snippets/function/functionVolIndexCoords");

    // each grid has 3 active voxels.  These vectors hold the expected values of those voxels
    // for each grid
    std::vector<openvdb::Vec3I> expectedVoxelVals(3);
    expectedVoxelVals[0] = openvdb::Vec3I(1, 1, -1);
    expectedVoxelVals[1] = openvdb::Vec3I(2, 10, 1);
    expectedVoxelVals[2] = openvdb::Vec3I(3, 3, 10);

    std::vector<openvdb::Int32Grid::Ptr> expectedGrids(3);

    for (size_t i = 0; i < 3; i++) {
        openvdb::Int32Grid::Ptr grid = openvdb::Int32Grid::create();
        grid->setTransform(transform);
        grid->setName("a" + std::to_string(i) + "_expected");

        openvdb::Int32Grid::Accessor accessor = grid->getAccessor();
        const openvdb::Vec3I& expectedVals = expectedVoxelVals[i];

        accessor.setValueOn(openvdb::Coord(1, 2 ,3), expectedVals[0]);
        accessor.setValueOn(openvdb::Coord(1, 10, 3), expectedVals[1]);
        accessor.setValueOn(openvdb::Coord(-1, 1, 10), expectedVals[2]);

        expectedGrids[i] = grid;
    }

    // check grids
    bool check = true;
    std::stringstream outMessage;
    for (size_t i = 0; i < 3; i++){
        std::stringstream stream;
        unittest_util::ComparisonSettings settings;
        unittest_util::ComparisonResult result(stream);

        check &= unittest_util::compareGrids(result, *testGrids[i], *expectedGrids[i], settings, nullptr);

        if (!check) outMessage << stream.str() << std::endl;
    }

    CPPUNIT_ASSERT_MESSAGE(outMessage.str(), check);
}

void
TestFunction::testFunctionVolumePWS()
{
    mHarness.testPoints(false);
    mHarness.addAttribute<openvdb::Vec3f>("a", openvdb::Vec3f(10.0f), openvdb::Vec3f(0.0f));
    mHarness.executeCode("test/snippets/function/functionVolumePWS");

    AXTESTS_STANDARD_ASSERT();
}

void
TestFunction::testFunctionDeletePoint()
{
    // NOTE: the "deletepoint" function doesn't actually directly delete points - it adds them
    // to the "dead" group which marks them for deletion afterwards

    mHarness.testVolumes(false);
    mHarness.addInputGroups({"dead"}, {false});
    mHarness.addExpectedGroups({"dead"}, {true});

    mHarness.executeCode("test/snippets/function/functionDeletePoint");

    AXTESTS_STANDARD_ASSERT();

    // test without existing dead group

    mHarness.reset();
    mHarness.addExpectedGroups({"dead"}, {true});

    mHarness.executeCode("test/snippets/function/functionDeletePoint");

    AXTESTS_STANDARD_ASSERT();
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
