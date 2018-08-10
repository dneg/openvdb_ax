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

/// @file codegen/PointComputeGenerator.h
///
/// @authors Nick Avramoussis
///
/// @brief  The visitor framework and function definition for point data
///         grid code generation
///

#ifndef OPENVDB_AX_POINT_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED
#define OPENVDB_AX_POINT_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

#include "ComputeGenerator.h"
#include "FunctionTypes.h"
#include "LeafLocalData.h"
#include "Types.h"
#include "Utils.h"

#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/TargetRegistry.h>

#include <openvdb/math/Transform.h>
#include <openvdb/points/AttributeGroup.h>
#include <openvdb/points/PointConversion.h>

#include <llvm/IR/Module.h>

#include <map>

// fwd declaration
namespace llvm
{
    struct Value;
}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

/// @brief  Base untyped handle struct for container storage
///
struct Handles { using UniquePtr = std::unique_ptr<Handles>; };

/// @brief  A wrapper around a VDB Points Attribute Handle, allowing for
///         typed storage of a read or write handle. This is used for
///         automatic memory management and void pointer passing into the
///         generated point functions
///
template <typename ValueT>
struct TypedHandle : public Handles
{
    using UniquePtr = std::unique_ptr<TypedHandle<ValueT>>;
    using HandleTraits = points::point_conversion_internal::ConversionTraits<ValueT>;
    using HandleT = typename HandleTraits::Handle;

    using LeafT = points::PointDataTree::LeafNodeType;

    inline void*
    initReadHandle(const LeafT& leaf, const size_t pos) {
        mHandle = HandleTraits::handleFromLeaf(const_cast<LeafT&>(leaf), pos);
        return static_cast<void*>(mHandle.get());
    }

    inline void*
    initWriteHandle(LeafT& leaf, const size_t pos) {
        mHandle = HandleTraits::writeHandleFromLeaf(leaf, pos);
        return static_cast<void*>(mHandle.get());
    }

private:
    typename HandleT::Ptr mHandle;
};

/// @brief  The function definition and signature which is built by the
///         PointComputeGenerator.
///
///         The argument structure is as follows:
///
///           1) - A void pointer to the CustomData
///           2) - A void pointer to the leaf AttributeSet
///           3) - A void pointer to an object of type openvdb::math::Transform,
///                representing the current grids transform
///           4) - A void pointer to an object of type openvdb::Coord,
///                representing the current voxel being executed
///           5) - An unsigned integer, representing the leaf relative point
///                id being executed
///           6) - A void pointer to a vector of void pointers, representing an
///                array of attribute handles
///           7) - A void pointer to a vector of void pointers, representing an
///                array of group handles
///           8) - A void pointer to a NewData object, used to track newly
///                initialized attributes and arrays
///
struct ComputePointFunction
{
    /// The name of the generated function
    static const std::string Name;

    /// The signature of the generated function
    using Signature =
        void(const void* const,
             const void* const,
             uint64_t,
             void**,
             void**,
             void*);

    using SignaturePtr = std::add_pointer<Signature>::type;
    using FunctionT = std::function<Signature>;
    using FunctionTraitsT = FunctionTraits<FunctionT>;
    using ReturnT = FunctionTraitsT::ReturnType;

    static const size_t N_ARGS = FunctionTraitsT::N_ARGS;

    /// The argument key names available during code generation
    static const std::array<std::string, N_ARGS> ArgumentKeys;

    /// @brief  The arguments of the generated function
    ///
    struct Arguments
    {
        Arguments(const CustomData& customData,
                  const points::AttributeSet& attributeSet,
                  const size_t pointCount)
            : mCustomData(&customData)
            , mAttributeSet(&attributeSet)
            , mIndex(0)
            , mLeafLocalData(new LeafLocalData(pointCount))
            , mVoidAttributeHandles()
            , mAttributeHandles()
            , mVoidGroupHandles()
            , mGroupHandles() {}

        /// @brief  Given a built version of the function signature, automatically
        ///         bind the current arguments and return a callable function
        ///         which takes no arguments
        ///
        /// @param  function  The fully generated function built from the
        ///                   PointComputeGenerator
        ///
        inline std::function<ReturnT()>
        bind(Signature function)
        {
            return std::bind(function,
                static_cast<FunctionTraitsT::Arg<0>::Type>(mCustomData),
                static_cast<FunctionTraitsT::Arg<1>::Type>(mAttributeSet),
                static_cast<FunctionTraitsT::Arg<2>::Type>(mIndex),
                static_cast<FunctionTraitsT::Arg<3>::Type>(mVoidAttributeHandles.data()),
                static_cast<FunctionTraitsT::Arg<4>::Type>(mVoidGroupHandles.data()),
                static_cast<FunctionTraitsT::Arg<5>::Type>(mLeafLocalData.get()));
        }

        template <typename ValueT>
        inline void
        addHandle(const points::PointDataTree::LeafNodeType& leaf,
                  const size_t pos)
        {
            typename TypedHandle<ValueT>::UniquePtr handle(new TypedHandle<ValueT>());
            mVoidAttributeHandles.emplace_back(handle->initReadHandle(leaf, pos));
            mAttributeHandles.emplace_back(std::move(handle));
        }

        template <typename ValueT>
        inline void
        addWriteHandle(points::PointDataTree::LeafNodeType& leaf,
                       const size_t pos)
        {
            typename TypedHandle<ValueT>::UniquePtr handle(new TypedHandle<ValueT>());
            mVoidAttributeHandles.emplace_back(handle->initWriteHandle(leaf, pos));
            mAttributeHandles.emplace_back(std::move(handle));
        }

        inline void
        addGroupHandle(const points::PointDataTree::LeafNodeType& leaf,
                       const std::string& name)
        {
            assert(leaf.attributeSet().descriptor().hasGroup(name));
            mGroupHandles.emplace_back(new points::GroupHandle(leaf.groupHandle(name)));
            mVoidGroupHandles.emplace_back(static_cast<void*>(mGroupHandles.back().get()));
        }

        inline void
        addGroupWriteHandle(points::PointDataTree::LeafNodeType& leaf,
                            const std::string& name)
        {
            assert(leaf.attributeSet().descriptor().hasGroup(name));
            mGroupHandles.emplace_back(new points::GroupWriteHandle(leaf.groupWriteHandle(name)));
            mVoidGroupHandles.emplace_back(static_cast<void*>(mGroupHandles.back().get()));
        }

        inline void addNullGroupHandle() { mVoidGroupHandles.emplace_back(nullptr); }

        const CustomData* const mCustomData;
        const points::AttributeSet* const mAttributeSet;
        uint64_t mIndex;
        LeafLocalData::UniquePtr mLeafLocalData;

    private:
        std::vector<void*> mVoidAttributeHandles;
        std::vector<Handles::UniquePtr> mAttributeHandles;
        std::vector<void*> mVoidGroupHandles;
        std::vector<points::GroupHandle::Ptr> mGroupHandles;
    };
};

/// @brief  An additonal function built by the PointComputeGenerator.
///         Currently both compute and compute range functions have the same
///         signature
struct ComputePointRangeFunction : public ComputePointFunction
{
    static const std::string Name;
};


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



/// @brief Visitor object which will generate llvm IR for a syntax tree which has been generated from
///        AX that targets point grids.  The IR will represent  2 functions : one that executes over
///        single points and one that executes over a collection of points.  This is primarily used by the
///        Compiler class.
struct PointComputeGenerator : public ComputeGenerator
{
    /// @brief Constructor
    /// @param module           llvm Module for generating IR
    /// @param customData       Custom data to be referenced from compiled AX code
    /// @param options          Options for the function registry behaviour
    /// @param functionRegistry Function registry object which will be used when generating IR
    ///                         for function calls
    /// @param warnings         Vector which will hold compiler warnings.  If null, no warnings will
    ///                         be stored.
    PointComputeGenerator(llvm::Module& module,
                          CustomData* customData,
                          const FunctionOptions& options,
                          FunctionRegistry& functionRegistry,
                          std::vector<std::string>* const warnings = nullptr);

    /// @brief Retrieves the names of the generated IR functions
    /// @param list Vector of strings into which the function names will be retrieved
    inline static void
    getFunctionList(std::vector<std::string>& list)
    {
        list.push_back(ComputePointFunction::Name);
        list.push_back(ComputePointRangeFunction::Name);
    }

    ~PointComputeGenerator() override = default;

    /// @brief initializes visitor.  Automatically called when visiting the tree's root node.
    void init(const ast::Tree& node) override;
    void visit(const ast::AssignExpression& node) override;
    void visit(const ast::Crement& node) override;
    void visit(const ast::FunctionCall& node) override;
    void visit(const ast::Attribute& node) override;
    void visit(const ast::AttributeValue& node) override;

private:

    // The string mapped function variables, defined by the Function interface
    SymbolTable mLLVMArguments;

    // Track how many attributes have been visisted so we can choose the correct
    // code path
    size_t mAttributeVisitCount;
};

}
}
}
}

#endif // OPENVDB_AX_POINT_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
