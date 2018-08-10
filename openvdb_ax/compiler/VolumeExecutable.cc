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

#include "VolumeExecutable.h"

// @TODO refactor so we don't have to include VolumeComputeGenerator.h, but still have the functions
// defined in one place
#include <openvdb_ax/codegen/VolumeComputeGenerator.h>
#include <openvdb_ax/Exceptions.h>

#include <openvdb/Exceptions.h>
#include <openvdb/tree/LeafManager.h>
#include <openvdb/Types.h>

#include <tbb/parallel_for.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {

namespace {

template <typename ValueType>
inline void
retrieveAccessorTyped(codegen::ComputeVolumeFunction::Arguments& args,
                      openvdb::GridBase::Ptr grid)
{
    using GridType = typename openvdb::BoolGrid::ValueConverter<ValueType>::Type;
    typename GridType::Ptr typed = openvdb::StaticPtrCast<GridType>(grid);
    args.addAccessor(typed->tree());
}

inline void
retrieveAccessor(codegen::ComputeVolumeFunction::Arguments& args,
                 const openvdb::GridBase::Ptr grid,
                 const std::string& valueType)
{
    if (valueType == typeNameAsString<bool>())                      retrieveAccessorTyped<bool>(args, grid);
    else if (valueType == typeNameAsString<int16_t>())              retrieveAccessorTyped<int16_t>(args, grid);
    else if (valueType == typeNameAsString<int32_t>())              retrieveAccessorTyped<int32_t>(args, grid);
    else if (valueType == typeNameAsString<int64_t>())              retrieveAccessorTyped<int64_t>(args, grid);
    else if (valueType == typeNameAsString<float>())                retrieveAccessorTyped<float>(args, grid);
    else if (valueType == typeNameAsString<double>())               retrieveAccessorTyped<double>(args, grid);
    else if (valueType == typeNameAsString<math::Vec3<int32_t>>())  retrieveAccessorTyped<math::Vec3<int32_t>>(args, grid);
    else if (valueType == typeNameAsString<math::Vec3<float>>())    retrieveAccessorTyped<math::Vec3<float>>(args, grid);
    else if (valueType == typeNameAsString<math::Vec3<double>>())   retrieveAccessorTyped<math::Vec3<double>>(args, grid);
    else {
        OPENVDB_THROW(TypeError, "Could not retrieve attribute '" + grid->getName()
            + "' as it has an unknown value type '" + valueType + "'");
    }
}

template <typename TreeT>
struct VolumeExecuterOp
{
    using LeafManagerT = typename tree::LeafManager<TreeT>;
    using FunctionT = codegen::ComputeVolumeFunction::SignaturePtr;

        VolumeExecuterOp(const VolumeRegistry& volumeRegistry,
                         const CustomData& customData,
                         const math::Transform& assignedVolumeTransform,
                         FunctionT computeFunction,
                         openvdb::GridPtrVec& grids)
        : mVolumeRegistry(volumeRegistry)
        , mCustomData(customData)
        , mComputeFunction(computeFunction)
        , mGrids(grids)
        , mTargetVolumeTransform(assignedVolumeTransform) {
            assert(!mGrids.empty());
        }

    void operator()(const typename LeafManagerT::LeafRange& range) const
    {
        codegen::ComputeVolumeFunction::Arguments args(mCustomData);

        size_t location(0);
        for (const auto& iter : mVolumeRegistry.volumeData()) {
            retrieveAccessor(args, mGrids[location], iter.mType);
            args.addTransform(mGrids[location]->transformPtr());
            ++location;
        }

        for (auto leaf = range.begin(); leaf; ++leaf) {
            for (auto voxel = leaf->cbeginValueOn(); voxel; ++voxel) {
                args.mCoord = voxel.getCoord();
                args.mCoordWS = mTargetVolumeTransform.indexToWorld(args.mCoord);
                args.bind(mComputeFunction)();
            }
        }
    }

private:
    const VolumeRegistry&       mVolumeRegistry;
    const CustomData&           mCustomData;
    FunctionT                   mComputeFunction;
    const openvdb::GridPtrVec&  mGrids;
    const math::Transform&      mTargetVolumeTransform;
};

void registerVolumes(const GridPtrVec &grids, GridPtrVec &writeableGrids, GridPtrVec &usableGrids,
                     const VolumeRegistry::VolumeDataVec& volumeData)
{
    for (auto& iter : volumeData) {

        openvdb::GridBase::Ptr matchedGrid;
        bool matchedName(false);
        for (const auto grid : grids) {
            if (grid->getName() != iter.mName) continue;
            matchedName = true;
            if (grid->valueType() != iter.mType) continue;
            matchedGrid = grid;
            break;
        }

        if (!matchedName && !matchedGrid) {
            OPENVDB_THROW(LookupError, "Missing grid \"@" + iter.mName + "\".");
        }

        if (matchedName && !matchedGrid) {
            OPENVDB_THROW(TypeError, "Mismatching grid access type. \"@" + iter.mName +
                "\" exists but has been accessed with type \"" + iter.mType + "\".");
        }

        assert(matchedGrid);
        usableGrids.push_back(matchedGrid);

        if (iter.mWriteable) {
            writeableGrids.push_back(matchedGrid);
        }
    }
}

} // anonymous namespace

void VolumeExecutable::execute(const openvdb::GridPtrVec& grids) const
{
    openvdb::GridPtrVec usableGrids, writeableGrids;

    registerVolumes(grids, writeableGrids, usableGrids, mVolumeRegistry->volumeData());

    using FunctionType = codegen::ComputeVolumeFunction;
    const int numBlocks = mBlockFunctionAddresses.size();

    for (int i = 0; i < numBlocks; i++) {

        FunctionType::SignaturePtr compute = nullptr;
        std::stringstream funcName("compute_volume_" + std::to_string(i));
        const std::map<std::string, uint64_t>& blockFunctions = mBlockFunctionAddresses.at(i);
        auto iter = blockFunctions.find(funcName.str());

        if (iter != blockFunctions.cend() && (iter->second != uint64_t(0))) {
            compute = reinterpret_cast<FunctionType::SignaturePtr>(iter->second);
        }

        if (!compute) {
            OPENVDB_THROW(AXCompilerError, "No code has been successfully compiled for execution.");
        }

        const std::string& currentVolumeAssigned = mAssignedVolumes[i];
        math::Transform::ConstPtr writeTransform = nullptr;

        // pointer to the grid which is being written to in the current block
        openvdb::GridBase::Ptr gridToModify = nullptr;

        for (const auto& grid : writeableGrids) {
            if (grid->getName() == currentVolumeAssigned) {
                writeTransform = grid->transformPtr();
                gridToModify = grid;
                break;
            }
        }

        // We execute over the topology of the grid currently being modified.  To do this, we need
        // a typed tree and leaf manager

        if (gridToModify->isType<BoolGrid>()) {
            BoolGrid::Ptr typed = StaticPtrCast<BoolGrid>(gridToModify);
            tree::LeafManager<BoolTree> leafManager(typed->tree());
            VolumeExecuterOp<BoolTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<Int32Grid>()) {
            Int32Grid::Ptr typed = StaticPtrCast<Int32Grid>(gridToModify);
            tree::LeafManager<Int32Tree> leafManager(typed->tree());
            VolumeExecuterOp<Int32Tree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                 compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<Int64Grid>()) {
            Int64Grid::Ptr typed = StaticPtrCast<Int64Grid>(gridToModify);
            tree::LeafManager<Int64Tree> leafManager(typed->tree());
            VolumeExecuterOp<Int64Tree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<FloatGrid>()) {
            FloatGrid::Ptr typed = StaticPtrCast<FloatGrid>(gridToModify);
            tree::LeafManager<FloatTree> leafManager(typed->tree());
            VolumeExecuterOp<FloatTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<DoubleGrid>()) {
            DoubleGrid::Ptr typed = StaticPtrCast<DoubleGrid>(gridToModify);
            tree::LeafManager<DoubleTree> leafManager(typed->tree());
            VolumeExecuterOp<DoubleTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<Vec3IGrid>()) {
            Vec3IGrid::Ptr typed = StaticPtrCast<Vec3IGrid>(gridToModify);
            tree::LeafManager<Vec3ITree> leafManager(typed->tree());
            VolumeExecuterOp<Vec3ITree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<Vec3fGrid>()) {
            Vec3fGrid::Ptr typed = StaticPtrCast<Vec3fGrid>(gridToModify);
            tree::LeafManager<Vec3fTree> leafManager(typed->tree());
            VolumeExecuterOp<Vec3fTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<Vec3dGrid>()) {
            Vec3dGrid::Ptr typed = StaticPtrCast<Vec3dGrid>(gridToModify);
            tree::LeafManager<Vec3dTree> leafManager(typed->tree());
            VolumeExecuterOp<Vec3dTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else if (gridToModify->isType<MaskGrid>()) {
            MaskGrid::Ptr typed = StaticPtrCast<MaskGrid>(gridToModify);
            tree::LeafManager<MaskTree> leafManager(typed->tree());
            VolumeExecuterOp<MaskTree> executerOp(*mVolumeRegistry, *mCustomData, *writeTransform,
                compute, usableGrids);
            tbb::parallel_for(leafManager.leafRange(), executerOp);
        }
        else {
            OPENVDB_THROW(TypeError, "Could not retrieve volume '" + gridToModify->getName()
                                     + "' as it has an unknown value type");
        }
    }
}

}
}
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
