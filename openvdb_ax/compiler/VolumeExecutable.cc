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

#include "VolumeExecutable.h"

#include <openvdb_ax/Exceptions.h>

// @TODO refactor so we don't have to include VolumeComputeGenerator.h,
// but still have the functions defined in one place
#include <openvdb_ax/codegen/VolumeComputeGenerator.h>

#include <openvdb/Exceptions.h>
#include <openvdb/tree/LeafManager.h>
#include <openvdb/Types.h>
#include <openvdb/math/Coord.h>
#include <openvdb/math/Transform.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/tree/ValueAccessor.h>
#include <openvdb/tree/LeafManager.h>

#include <tbb/parallel_for.h>

#include <memory>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {

namespace {

/// @brief Volume Kernel types
///
using KernelFunctionPtr = std::add_pointer<codegen::VolumeKernel::Signature>::type;
using FunctionTraitsT = codegen::VolumeKernel::FunctionTraitsT;
using ReturnT = FunctionTraitsT::ReturnType;


/// The arguments of the generated function
struct VolumeFunctionArguments
{
    struct Accessors
    {
        using UniquePtr = std::unique_ptr<Accessors>;
        virtual ~Accessors() = default;
    };

    template <typename TreeT>
    struct TypedAccessor final : public Accessors
    {
        using UniquePtr = std::unique_ptr<TypedAccessor<TreeT>>;
        TypedAccessor(TreeT& tree)
            : mAccessor(new tree::ValueAccessor<TreeT>(tree)) {}
        ~TypedAccessor() override final = default;

        inline void* get() const { return static_cast<void*>(mAccessor.get()); }
        const std::unique_ptr<tree::ValueAccessor<TreeT>> mAccessor;
    };


    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////


    VolumeFunctionArguments(const CustomData::ConstPtr customData)
        : mCustomData(customData)
        , mCoord()
        , mCoordWS()
        , mIdx(0)
        , mAccessor()
        , mVoidAccessors()
        , mAccessors()
        , mVoidTransforms() {}

    /// @brief  Given a built version of the function signature, automatically
    ///         bind the current arguments and return a callable function
    ///         which takes no arguments
    ///
    /// @param  function  The fully generated function built from the
    ///                   VolumeComputeGenerator
    ///
    inline std::function<ReturnT()>
    bind(KernelFunctionPtr function)
    {
        return std::bind(function,
            static_cast<FunctionTraitsT::Arg<0>::Type>(mCustomData.get()),
            reinterpret_cast<FunctionTraitsT::Arg<1>::Type>(mCoord.data()),
            reinterpret_cast<FunctionTraitsT::Arg<2>::Type>(mCoordWS.asV()),
            static_cast<FunctionTraitsT::Arg<3>::Type>(mVoidAccessors.data()),
            static_cast<FunctionTraitsT::Arg<4>::Type>(mVoidTransforms.data()),
            static_cast<FunctionTraitsT::Arg<5>::Type>(mIdx),
            mAccessor);
    }

    template <typename TreeT>
    inline void
    addAccessor(TreeT& tree)
    {
        typename TypedAccessor<TreeT>::UniquePtr accessor(new TypedAccessor<TreeT>(tree));
        mVoidAccessors.emplace_back(accessor->get());
        mAccessors.emplace_back(std::move(accessor));
    }

    inline void
    addTransform(math::Transform::Ptr transform)
    {
        mVoidTransforms.emplace_back(static_cast<void*>(transform.get()));
    }

    const CustomData::ConstPtr mCustomData;
    openvdb::Coord mCoord;
    openvdb::math::Vec3<float> mCoordWS;
    size_t mIdx;
    void *mAccessor;

private:
    std::vector<void*> mVoidAccessors;
    std::vector<Accessors::UniquePtr> mAccessors;
    std::vector<void*> mVoidTransforms;
};

template <typename GridT>
inline void
retrieveAccessorTyped(VolumeFunctionArguments& args,
                      openvdb::GridBase::Ptr grid)
{
    typename GridT::Ptr typed = openvdb::StaticPtrCast<GridT>(grid);
    args.addAccessor(typed->tree());
}

inline bool supported(const ast::tokens::CoreType type)
{
    switch (type) {
        case ast::tokens::BOOL    : return true;
        case ast::tokens::INT     : return true;
        case ast::tokens::LONG    : return true;
        case ast::tokens::FLOAT   : return true;
        case ast::tokens::DOUBLE  : return true;
        case ast::tokens::VEC3I   : return true;
        case ast::tokens::VEC3F   : return true;
        case ast::tokens::VEC3D   : return true;
        case ast::tokens::UNKNOWN :
        default                   :
            return false;
    }
}

inline void
retrieveAccessor(VolumeFunctionArguments& args,
                 const openvdb::GridBase::Ptr grid,
                 const ast::tokens::CoreType& type)
{
    // assert so the executer can be marked as noexcept (assuming nothing throws in compute)
    assert(supported(type) && "Could not retrieve accessor from unsupported type");
    switch (type) {
        case ast::tokens::BOOL    : return retrieveAccessorTyped<BoolGrid>(args, grid);
        case ast::tokens::INT     : return retrieveAccessorTyped<Int32Grid>(args, grid);
        case ast::tokens::LONG    : return retrieveAccessorTyped<Int64Grid>(args, grid);
        case ast::tokens::FLOAT   : return retrieveAccessorTyped<FloatGrid>(args, grid);
        case ast::tokens::DOUBLE  : return retrieveAccessorTyped<DoubleGrid>(args, grid);
        case ast::tokens::VEC3I   : return retrieveAccessorTyped<Vec3IGrid>(args, grid);
        case ast::tokens::VEC3F   : return retrieveAccessorTyped<Vec3fGrid>(args, grid);
        case ast::tokens::VEC3D   : return retrieveAccessorTyped<Vec3dGrid>(args, grid);
        case ast::tokens::UNKNOWN :
        default                   : return;
    }
}


inline openvdb::GridBase::Ptr
createGrid(const ast::tokens::CoreType& type)
{
    // assert so the executer can be marked as noexcept (assuming nothing throws in compute)
    assert(supported(type) && "Could not retrieve accessor from unsupported type");
    switch (type) {
        case ast::tokens::BOOL    : return openvdb::BoolGrid::create();
        case ast::tokens::INT     : return openvdb::Int32Grid::create();
        case ast::tokens::LONG    : return openvdb::Int64Grid::create();
        case ast::tokens::FLOAT   : return openvdb::FloatGrid::create();
        case ast::tokens::DOUBLE  : return openvdb::DoubleGrid::create();
        case ast::tokens::VEC3I   : return openvdb::Vec3IGrid::create();
        case ast::tokens::VEC3F   : return openvdb::Vec3fGrid::create();
        case ast::tokens::VEC3D   : return openvdb::Vec3dGrid::create();
        case ast::tokens::UNKNOWN :
        default                   : return nullptr;
    }
}

template <typename TreeT, typename ValueIterT>
struct VolumeExecuterOp
{
    using LeafManagerT = typename tree::LeafManager<TreeT>;
    using LeafNodeT = typename TreeT::LeafNodeType;
    using IterTraitsT = typename tree::IterTraits<LeafNodeT, ValueIterT>;

    VolumeExecuterOp(const AttributeRegistry& attributeRegistry,
                     const CustomData::ConstPtr& customData,
                     const math::Transform& assignedVolumeTransform,
                     KernelFunctionPtr computeFunction,
                     const openvdb::GridPtrVec& grids,
                     openvdb::tree::ValueAccessor<TreeT> acc,
                     const size_t idx)
        : mAttributeRegistry(attributeRegistry)
        , mCustomData(customData)
        , mComputeFunction(computeFunction)
        , mGrids(grids)
        , mTargetVolumeTransform(assignedVolumeTransform)
        , mIdx(idx)
        , mAccessor(acc) {
            assert(!mGrids.empty());
        }

    void operator()(const typename LeafManagerT::LeafRange& range) const
    {
        VolumeFunctionArguments args(mCustomData);
        args.mIdx = mIdx;
        args.mAccessor = const_cast<void*>(static_cast<const void*>(&mAccessor));

        size_t location(0);
        for (const auto& iter : mAttributeRegistry.data()) {
            retrieveAccessor(args, mGrids[location], iter.type());
            args.addTransform(mGrids[location]->transformPtr());
            ++location;
        }

        for (auto leaf = range.begin(); leaf; ++leaf) {
            for (ValueIterT voxel = IterTraitsT::begin(*leaf); voxel; ++voxel) {
                args.mCoord = voxel.getCoord();
                args.mCoordWS = mTargetVolumeTransform.indexToWorld(args.mCoord);
                args.bind(mComputeFunction)();
            }
        }
    }

private:
    const AttributeRegistry&    mAttributeRegistry;
    const CustomData::ConstPtr  mCustomData;
    KernelFunctionPtr           mComputeFunction;
    const openvdb::GridPtrVec&  mGrids;
    const math::Transform&      mTargetVolumeTransform;
    const size_t mIdx;
    openvdb::tree::ValueAccessor<TreeT> mAccessor;


};

void registerVolumes(GridPtrVec& grids,
    GridPtrVec& writeableGrids,
    GridPtrVec& readGrids,
    const AttributeRegistry& registry,
    const bool createMissing)
{
    for (auto& iter : registry.data()) {

        openvdb::GridBase::Ptr matchedGrid;
        bool matchedName(false);
        ast::tokens::CoreType type = ast::tokens::UNKNOWN;

        for (const auto grid : grids) {
            if (grid->getName() != iter.name()) continue;
            matchedName = true;
            type = ast::tokens::tokenFromTypeString(grid->valueType());
            if (type != iter.type()) continue;
            matchedGrid = grid;
            break;
        }

        if (createMissing && !matchedGrid) {
            matchedGrid = createGrid(iter.type());
            if (matchedGrid) {
                matchedGrid->setName(iter.name());
                grids.emplace_back(matchedGrid);
                matchedName = true;
                type = iter.type();
            }
        }
        if (!matchedName && !matchedGrid) {
            OPENVDB_THROW(LookupError, "Missing grid \"@" + iter.name() + "\".");
        }
        if (matchedName && !matchedGrid) {
            OPENVDB_THROW(TypeError, "Mismatching grid access type. \"@" + iter.name() +
                "\" exists but has been accessed with type \"" +
                ast::tokens::typeStringFromToken(iter.type()) + "\".");
        }

        assert(matchedGrid);

        if (!supported(type)) {
            OPENVDB_THROW(TypeError, "Could not register volume '"
                + matchedGrid->getName() + "' as it has an unknown or unsupported value type '"
                + matchedGrid->valueType() + "'");
        }

        // Populate the write/read grids based on the access registry. If a
        // grid is being written to and has non self usage, (influences
        // another grids value which isn't it's own) it must be deep copied

        // @todo implement better execution order detection which could minimize
        // the number of deep copies required

        if (iter.writes() && iter.affectsothers()) {
            // if affectsothers(), it's also read from at some point
            assert(iter.reads());
            readGrids.push_back(matchedGrid->deepCopyGrid());
            writeableGrids.push_back(matchedGrid);
        }
        else {
            if (iter.writes()) {
                writeableGrids.push_back(matchedGrid);
            }
            readGrids.push_back(matchedGrid);
        }
    }
}

template<typename LeafT>
struct ValueOnIter { using Iterator = typename LeafT::ValueOnIter; };

template<typename LeafT>
struct ValueAllIter { using Iterator = typename LeafT::ValueAllIter; };

template<typename LeafT>
struct ValueOffIter { using Iterator = typename LeafT::ValueOffIter; };

template <template <typename> class IterT, typename GridT>
inline void run(openvdb::GridBase& grid,
    const openvdb::GridPtrVec& readGrids,
    KernelFunctionPtr kernel,
    const AttributeRegistry::ConstPtr registry,
    const CustomData::ConstPtr custom)
{
    using TreeType = typename GridT::TreeType;
    using LeafNodeType = typename TreeType::LeafNodeType;
    using IterType = IterT<LeafNodeType>;

    const ast::tokens::CoreType type =
            ast::tokens::tokenFromTypeString(grid.valueType());
    const int64_t idx = registry->accessIndex(grid.getName(), type);
    assert(idx != -1);

    GridT& typed = static_cast<GridT&>(grid);
    tree::LeafManager<TreeType> leafManager(typed.tree());
    VolumeExecuterOp<TreeType, typename IterType::Iterator>
        executerOp(*registry, custom, grid.transform(),
            kernel, readGrids, typed.getAccessor(), idx);

    tbb::parallel_for(leafManager.leafRange(), executerOp);
}

template <template <typename> class IterT>
inline void run(const openvdb::GridPtrVec& writeableGrids,
                const openvdb::GridPtrVec& readGrids,
                KernelFunctionPtr kernel,
                const AttributeRegistry::ConstPtr registry,
                const CustomData::ConstPtr custom)
{
    for (const auto& grid : writeableGrids) {

        // We execute over the topology of the grid currently being modified.  To do this, we need
        // a typed tree and leaf manager

        if (grid->isType<BoolGrid>()) {
            run<IterT, BoolGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<Int32Grid>()) {
            run<IterT, Int32Grid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<Int64Grid>()) {
            run<IterT, Int64Grid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<FloatGrid>()) {
            run<IterT, FloatGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<DoubleGrid>()) {
            run<IterT, DoubleGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<Vec3IGrid>()) {
            run<IterT, Vec3IGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<Vec3fGrid>()) {
            run<IterT, Vec3fGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<Vec3dGrid>()) {
            run<IterT, Vec3dGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else if (grid->isType<MaskGrid>()) {
            run<IterT, MaskGrid>(*grid, readGrids, kernel, registry, custom);
        }
        else {
            OPENVDB_THROW(TypeError, "Could not retrieve volume '" + grid->getName()
                + "' as it has an unknown or unsupported value type '" + grid->valueType()
                + "'");
        }
    }
}
} // anonymous namespace

void VolumeExecutable::execute(openvdb::GridPtrVec& grids,
                               const IterType iterType,
                               const bool createMissing) const
{
    openvdb::GridPtrVec readGrids, writeableGrids;

    registerVolumes(grids, writeableGrids, readGrids, *mAttributeRegistry, createMissing);

    KernelFunctionPtr kernel = nullptr;
    const auto iter = mFunctionAddresses.find(codegen::VolumeKernel::getDefaultName());
    if (iter != mFunctionAddresses.end()) {
        kernel = reinterpret_cast<KernelFunctionPtr>(iter->second);
    }
    if (!kernel) {
        OPENVDB_THROW(AXCompilerError,
            "No code has been successfully compiled for execution.");
    }

    if (iterType == IterType::ON)       run<ValueOnIter>(writeableGrids, readGrids, kernel, mAttributeRegistry, mCustomData);
    else if (iterType == IterType::OFF) run<ValueOffIter>(writeableGrids, readGrids, kernel, mAttributeRegistry, mCustomData);
    else if (iterType == IterType::ALL) run<ValueAllIter>(writeableGrids, readGrids, kernel, mAttributeRegistry, mCustomData);
    else {
        OPENVDB_THROW(AXExecutionError,
            "Unrecognised voxel iterator.");
    }
}


}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
