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

#include <openvdb_ax/compiler/CustomData.h>

#include <boost/functional/hash.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_01.hpp>

#include <cppunit/extensions/HelperMacros.h>

using namespace openvdb::points;
using namespace openvdb::ax;

class TestChannelExpressions : public CppUnit::TestCase
{
public:
    CPPUNIT_TEST_SUITE(TestChannelExpressions);
    CPPUNIT_TEST(testSetChannelExpressionValues);
    CPPUNIT_TEST_SUITE_END();

    void testSetChannelExpressionValues();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestChannelExpressions);

void
TestChannelExpressions::testSetChannelExpressionValues()
{
    unittest_util::AXTestHarness harness1;

    harness1.addAttribute<float>("foo", 2.0f);
    harness1.addAttribute<openvdb::Vec3f>("v", openvdb::Vec3f(1.0f, 2.0f, 3.0f));

    using FloatMeta = openvdb::TypedMetadata<float>;
    using VectorFloatMeta = openvdb::TypedMetadata<openvdb::math::Vec3<float>>;

    FloatMeta customFloatData(2.0f);
    VectorFloatMeta customVecData(openvdb::math::Vec3<float>(1.0f, 2.0f, 3.0f));

    // test initialising the data before compile

    CustomData::Ptr pointData = CustomData::create();
    CustomData::Ptr volumeData = CustomData::create();

    pointData->insertData("float1", customFloatData.copy());
    pointData->insertData("vector1", customVecData.copy());
    volumeData->insertData("float1", customFloatData.copy());
    volumeData->insertData("vector1", customVecData.copy());

    const std::string code = unittest_util::loadText("test/snippets/channel/channelExpressionSet");
    Compiler compiler;
    PointExecutable::Ptr pointExecutable = compiler.compile<PointExecutable>(code, pointData);
    VolumeExecutable::Ptr volumeExecutable = compiler.compile<VolumeExecutable>(code, volumeData);

    for (auto& grid : harness1.mInputPointGrids) {
        pointExecutable->execute(*grid.second);
    }

    for (auto& grid : harness1.mInputVolumeGrids) {
        volumeExecutable->execute(grid.second);
    }

    AXTESTS_STANDARD_ASSERT_HARNESS(harness1)

    unittest_util::AXTestHarness harness2;

    harness2.addAttribute<float>("foo", 2.0f);
    harness2.addAttribute<openvdb::Vec3f>("v", openvdb::Vec3f(1.0f, 2.0f, 3.0f));

    pointData->reset();
    volumeData->reset();

    // recompile
    pointExecutable = compiler.compile<PointExecutable>(code, pointData);
    volumeExecutable = compiler.compile<VolumeExecutable>(code, volumeData);

    pointData->insertData("float1", customFloatData.copy());
    VectorFloatMeta::Ptr customTypedVecData =
        openvdb::StaticPtrCast<VectorFloatMeta>(customVecData.copy());
    pointData->insertData<VectorFloatMeta>("vector1", customTypedVecData);

    volumeData->insertData("float1", customFloatData.copy());
    volumeData->insertData<VectorFloatMeta>("vector1", customTypedVecData);

    for (auto& grid : harness2.mInputPointGrids) {
        pointExecutable->execute(*grid.second);
    }

    for (auto& grid : harness2.mInputVolumeGrids) {
        volumeExecutable->execute(grid.second);
    }

    AXTESTS_STANDARD_ASSERT_HARNESS(harness2)
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
