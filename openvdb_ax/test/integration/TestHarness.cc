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

#include <openvdb/points/PointConversion.h>

namespace unittest_util
{

std::string loadText(const std::string& codeFileName)
{
    std::ostringstream sstream;
    std::ifstream fs(codeFileName);

    if (fs.fail()) {
        throw std::runtime_error(std::string("Failed to open ") + std::string(codeFileName));
    }

    sstream << fs.rdbuf();
    return sstream.str();
}

void wrapExecution(openvdb::points::PointDataGrid& grid,
                   const std::string& codeFileName,
                   const std::string * const group,
                   std::vector<std::string>* warnings,
                   const openvdb::ax::CustomData::Ptr& data)
{
    using namespace openvdb::ax;

    Compiler compiler;
    const std::string code = loadText(codeFileName);
    ast::Tree::Ptr syntaxTree = ast::parse(code.c_str());
    PointExecutable::Ptr executable = compiler.compile<PointExecutable>(*syntaxTree, data, warnings);
    executable->execute(grid, group);
}

void wrapExecution(openvdb::GridPtrVec& grids,
                   const std::string& codeFileName,
                   std::vector<std::string>* warnings,
                   const openvdb::ax::CustomData::Ptr& data)
{
    using namespace openvdb::ax;

    Compiler compiler;
    const std::string code = loadText(codeFileName);
    ast::Tree::Ptr syntaxTree = ast::parse(code.c_str());
    VolumeExecutable::Ptr executable = compiler.compile<VolumeExecutable>(*syntaxTree, data, warnings);
    executable->execute(grids);
}

void AXTestHarness::addInputGroups(const std::vector<std::string> &names,
                                   const std::vector<bool> &defaults)
{
    for (size_t i = 0; i < names.size(); i++) {
        for (auto& grid : mInputPointGrids) {
            openvdb::points::appendGroup(grid.second->tree(), names[i]);
            openvdb::points::setGroup(grid.second->tree(), names[i], defaults[i]);
        }
    }
}

void AXTestHarness::addExpectedGroups(const std::vector<std::string> &names,
                                      const std::vector<bool> &defaults)
{
    for (size_t i = 0; i < names.size(); i++) {
        for (auto& grid : mOutputPointGrids) {
            openvdb::points::appendGroup(grid.second->tree(), names[i]);
            openvdb::points::setGroup(grid.second->tree(), names[i], defaults[i]);
        }
    }
}

void AXTestHarness::executeCode(const std::string& codeFile,
                                const std::string * const group,
                                std::vector<std::string>* warnings,
                                const openvdb::ax::CustomData::Ptr& data)
{
    if (mUsePoints) {
        for (auto& grid : mInputPointGrids) {
            wrapExecution(*(grid.second), codeFile, group, warnings, data);
        }
    }

    if (mUseVolumes)
    {
        for (auto& grids : mInputVolumeGrids) {
            wrapExecution(grids.second, codeFile, warnings, data);
        }
    }
}

bool AXTestHarness::checkAgainstExpected(std::ostream& sstream)
{
    unittest_util::ComparisonSettings settings;
    bool success = true;

    if (mUsePoints) {
        std::stringstream resultStream;
        unittest_util::ComparisonResult result(resultStream);

        const bool pointSuccess = unittest_util::compareGrids(result, *mOutputPointGrids["four_point"],
            *mInputPointGrids["four_point"], settings, nullptr);
        if (!pointSuccess)   sstream << resultStream.str() << std::endl;
        success &= pointSuccess;
    }

    if (mUseVolumes) {
        for (size_t i = 0; i < mInputVolumeGrids["one_voxel"].size(); i++) {
            std::stringstream resultStream;
            unittest_util::ComparisonResult result(resultStream);

            const bool volumeSuccess =
                unittest_util::compareUntypedGrids(result, *mOutputVolumeGrids["one_voxel"][i],
                    *mInputVolumeGrids["one_voxel"][i], settings, nullptr);
            success &= volumeSuccess;
            if (!volumeSuccess)  sstream << resultStream.str() << std::endl;
        }
    }

    return success;
}

void AXTestHarness::testVolumes(const bool enable)
{
    mUseVolumes = enable;
}

void AXTestHarness::testPoints(const bool enable)
{
    mUsePoints = enable;
}

void AXTestHarness::reset()
{
    using openvdb::points::PointDataGrid;
    using openvdb::points::NullCodec;

    std::vector<openvdb::Vec3d> coordinates =
        {openvdb::Vec3d(0.0, 0.0, 0.0),
         openvdb::Vec3d(0.0, 0.0, 0.05),
         openvdb::Vec3d(0.0, 1.0, 0.0),
         openvdb::Vec3d(1.0, 1.0, 0.0)};

    openvdb::math::Transform::Ptr transform1 = openvdb::math::Transform::createLinearTransform(1.0);
    openvdb::points::PointDataGrid::Ptr onePointGrid =
        openvdb::points::createPointDataGrid<NullCodec, PointDataGrid>(std::vector<openvdb::Vec3d>{coordinates[0]},
            *transform1);

    onePointGrid->setName("one_point");
    mInputPointGrids["one_point"] = onePointGrid;

    mOutputPointGrids["one_point"] = onePointGrid->deepCopy();
    mOutputPointGrids["one_point"]->setName("one_point_expected");

    openvdb::math::Transform::Ptr transform2 = openvdb::math::Transform::createLinearTransform(0.1);
    openvdb::points::PointDataGrid::Ptr fourPointGrid =
        openvdb::points::createPointDataGrid<NullCodec, PointDataGrid>(coordinates, *transform2);

    fourPointGrid->setName("four_points");
    mInputPointGrids["four_point"] = fourPointGrid;

    mOutputPointGrids["four_point"] = fourPointGrid->deepCopy();
    mOutputPointGrids["four_point"]->setName("four_points_expected");

    mInputVolumeGrids["one_voxel"].clear();
    mOutputVolumeGrids["one_voxel"].clear();
}

void AXTestCase::setUp()
{
    mHarness.reset();
}

}


// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
