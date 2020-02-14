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
                   const openvdb::ax::CustomData::Ptr& data,
                   const openvdb::ax::CompilerOptions& opts)
{
    using namespace openvdb::ax;

    Compiler compiler(opts);
    const std::string code = loadText(codeFileName);
    ast::Tree::Ptr syntaxTree = ast::parse(code.c_str());
    PointExecutable::Ptr executable = compiler.compile<PointExecutable>(*syntaxTree, data, warnings);
    executable->execute(grid, group);
}

void wrapExecution(openvdb::GridPtrVec& grids,
                   const std::string& codeFileName,
                   std::vector<std::string>* warnings,
                   const openvdb::ax::CustomData::Ptr& data,
                   const openvdb::ax::CompilerOptions& opts)
{
    using namespace openvdb::ax;

    Compiler compiler(opts);
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
            openvdb::points::appendGroup(grid->tree(), names[i]);
            openvdb::points::setGroup(grid->tree(), names[i], defaults[i]);
        }
    }
}

void AXTestHarness::addExpectedGroups(const std::vector<std::string> &names,
                                      const std::vector<bool> &defaults)
{
    for (size_t i = 0; i < names.size(); i++) {
        for (auto& grid : mOutputPointGrids) {
            openvdb::points::appendGroup(grid->tree(), names[i]);
            openvdb::points::setGroup(grid->tree(), names[i], defaults[i]);
        }
    }
}

void AXTestHarness::executeCode(const std::string& codeFile,
                                const std::string * const group,
                                std::vector<std::string>* warnings,
                                const openvdb::ax::CustomData::Ptr& data,
                                const openvdb::ax::CompilerOptions& opts)
{
    if (mUsePoints) {
        for (auto& grid : mInputPointGrids) {
            wrapExecution(*grid, codeFile, group, warnings, data, opts);
        }
    }

    if (mUseVolumes)
    {
        wrapExecution(mInputVolumeGrids, codeFile, warnings, data, opts);
    }
}

bool AXTestHarness::checkAgainstExpected(std::ostream& sstream)
{
    unittest_util::ComparisonSettings settings;
    bool success = true;

    if (mUsePoints) {
        std::stringstream resultStream;
        unittest_util::ComparisonResult result(resultStream);

        const size_t count = mInputPointGrids.size();
        for (size_t i = 0; i < count; ++i) {
            const auto& input = mInputPointGrids[i];
            const auto& expected = mOutputPointGrids[i];
            const bool pass =
                unittest_util::compareGrids(result, *expected, *input, settings, nullptr);
            if (!pass) sstream << resultStream.str() << std::endl;
            success &= pass;
        }
    }

    if (mUseVolumes) {
        for (size_t i = 0; i < mInputVolumeGrids.size(); i++) {
            std::stringstream resultStream;
            unittest_util::ComparisonResult result(resultStream);
            const bool volumeSuccess =
                unittest_util::compareUntypedGrids(result, *mOutputVolumeGrids[i],
                    *mInputVolumeGrids[i], settings, nullptr);
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

void AXTestHarness::reset(const openvdb::Index64 ppv, const openvdb::CoordBBox& bounds)
{
    using openvdb::points::PointDataGrid;
    using openvdb::points::NullCodec;

    mInputPointGrids.clear();
    mOutputPointGrids.clear();
    mInputVolumeGrids.clear();
    mOutputVolumeGrids.clear();

    openvdb::math::Transform::Ptr transform =
        openvdb::math::Transform::createLinearTransform(1.0);
    openvdb::MaskGrid::Ptr mask = openvdb::MaskGrid::create();
    mask->setTransform(transform);
    mask->sparseFill(bounds, true, true);
    openvdb::points::PointDataGrid::Ptr points =
        openvdb::points::denseUniformPointScatter(*mask, static_cast<float>(ppv));
    mask.reset();

    mInputPointGrids.emplace_back(points);
    mOutputPointGrids.emplace_back(points->deepCopy());
    mOutputPointGrids.back()->setName("custom_expected");

    mVolumeBounds = bounds;
}

void AXTestHarness::reset()
{
    using openvdb::points::PointDataGrid;
    using openvdb::points::NullCodec;

    mInputPointGrids.clear();
    mOutputPointGrids.clear();
    mInputVolumeGrids.clear();
    mOutputVolumeGrids.clear();

    std::vector<openvdb::Vec3d> coordinates =
        {openvdb::Vec3d(0.0, 0.0, 0.0),
         openvdb::Vec3d(0.0, 0.0, 0.05),
         openvdb::Vec3d(0.0, 1.0, 0.0),
         openvdb::Vec3d(1.0, 1.0, 0.0)};

    openvdb::math::Transform::Ptr transform1 =
        openvdb::math::Transform::createLinearTransform(1.0);

    openvdb::points::PointDataGrid::Ptr onePointGrid =
        openvdb::points::createPointDataGrid<NullCodec, PointDataGrid>
            (std::vector<openvdb::Vec3d>{coordinates[0]}, *transform1);

    onePointGrid->setName("1_point");
    mInputPointGrids.emplace_back(onePointGrid);
    mOutputPointGrids.emplace_back(onePointGrid->deepCopy());
    mOutputPointGrids.back()->setName("1_point_expected");

    openvdb::math::Transform::Ptr transform2 =
        openvdb::math::Transform::createLinearTransform(0.1);

    openvdb::points::PointDataGrid::Ptr fourPointGrid =
        openvdb::points::createPointDataGrid<NullCodec, PointDataGrid>
            (coordinates, *transform2);

    fourPointGrid->setName("4_points");
    mInputPointGrids.emplace_back(fourPointGrid);
    mOutputPointGrids.emplace_back(fourPointGrid->deepCopy());
    mOutputPointGrids.back()->setName("4_points_expected");

    mVolumeBounds = openvdb::CoordBBox({0,0,0}, {0,0,0});
}

template <typename ValueT>
using ConverterT = typename openvdb::BoolGrid::ValueConverter<ValueT>::Type;

void AXTestHarness::resetInputsToZero()
{
    for (auto& grid : mInputPointGrids) {
        openvdb::tree::LeafManager<openvdb::points::PointDataTree> manager(grid->tree());
        manager.foreach([](openvdb::points::PointDataTree::LeafNodeType& leaf, size_t) {
            const size_t attrs = leaf.attributeSet().size();
            const size_t pidx = leaf.attributeSet().descriptor().find("P");
            for (size_t idx = 0; idx < attrs; ++idx) {
                if (idx == pidx) continue;
                leaf.attributeArray(idx).collapse();
            }
        });
    }

    using AllowedGridTypes =
        openvdb::TypeList<
            openvdb::BoolGrid,
            openvdb::DoubleGrid,
            openvdb::FloatGrid,
            openvdb::Int32Grid,
            openvdb::Int64Grid,
            openvdb::MaskGrid,
            openvdb::StringGrid,
            openvdb::Vec3IGrid,
            openvdb::Vec3dGrid,
            openvdb::Vec3fGrid,
            ConverterT<openvdb::math::Vec4<int32_t>>,
            ConverterT<openvdb::math::Vec4<float>>,
            ConverterT<openvdb::math::Vec4<double>>>;

    for (auto& grid : mInputVolumeGrids) {
        const bool success = grid->apply<AllowedGridTypes>([](auto& typed) {
            using GridType = typename std::decay<decltype(typed)>::type;
            openvdb::tree::LeafManager<typename GridType::TreeType> manager(typed.tree());
            manager.foreach([](typename GridType::TreeType::LeafNodeType& leaf, size_t) {
                leaf.fill(typename GridType::ValueType(0));
            });
        });
        if (!success) {
            throw std::runtime_error("Unable to reset input grid of an unsupported type");
        }
    }
}


void AXTestCase::setUp()
{
    mHarness.reset();
}

}


// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
