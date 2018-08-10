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

/// @file codegen/VolumeComputeGenerator.h
///
/// @authors Nick Avramoussis
///
/// @brief  The visitor framework and function definition for volume grid
///         code generation
///

#ifndef OPENVDB_AX_VOLUME_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED
#define OPENVDB_AX_VOLUME_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

#include "ComputeGenerator.h"
#include "FunctionTypes.h"

#include <openvdb_ax/compiler/TargetRegistry.h>

#include <openvdb/math/Coord.h>
#include <openvdb/math/Transform.h>
#include <openvdb/math/Vec3.h>
#include <openvdb/tree/ValueAccessor.h>

#include <llvm/IR/Module.h>

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

struct Accessors { using Ptr = std::unique_ptr<Accessors>; };

template <typename TreeT>
struct TypedAccessor : public Accessors
{
    using Ptr = std::unique_ptr<TypedAccessor<TreeT>>;

    inline void*
    init(TreeT& tree) {
        mAccessor.reset(new tree::ValueAccessor<TreeT>(tree));
        return static_cast<void*>(mAccessor.get());
    }

    std::unique_ptr<tree::ValueAccessor<TreeT>> mAccessor;
};

/// @brief  The function definition and signature which is built by the
///         VolumeComputeGenerator.
///
///         The argument structure is as follows:
///
///             1) - A void pointer to the CustomData
///             2) - An pointer to an array of three ints representing the
///                  current voxel coord being accessed
///             3) - An pointer to an array of three floats representing the
///                  current voxel world space coord being accessed
///             4) - A void pointer to a vector of void pointers, representing an array
///                  of grid accessors
///             4) - A void pointer to a vector of void pointers, representing an array
///                  of grid transforms
///
struct ComputeVolumeFunction
{
    /// The name of the generated function
    static const std::string DefaultName;

    /// The signature of the generated function
    using Signature =
        void(const void* const,
             const int32_t (*)[3],
             const float (*)[3],
             void**,
             void**
            );

    using SignaturePtr = std::add_pointer<Signature>::type;
    using FunctionT = std::function<Signature>;
    using FunctionTraitsT = FunctionTraits<FunctionT>;
    using ReturnT = FunctionTraitsT::ReturnType;

    static const size_t N_ARGS = FunctionTraitsT::N_ARGS;

    /// The argument key names available during code generation
    static const std::array<std::string, N_ARGS> ArgumentKeys;

    /// The arguments of the generated function
    struct Arguments
    {
        Arguments(const CustomData& customData)
            : mCustomDataPtr(&customData)
            , mCoord()
            , mCoordWS()
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
        bind(Signature function)
        {
            return std::bind(function,
                static_cast<FunctionTraitsT::Arg<0>::Type>(mCustomDataPtr),
                reinterpret_cast<FunctionTraitsT::Arg<1>::Type>(mCoord.data()),
                reinterpret_cast<FunctionTraitsT::Arg<2>::Type>(mCoordWS.asV()),
                static_cast<FunctionTraitsT::Arg<3>::Type>(mVoidAccessors.data()),
                static_cast<FunctionTraitsT::Arg<4>::Type>(mVoidTransforms.data()));
        }

        template <typename TreeT>
        inline void
        addAccessor(TreeT& tree)
        {
            typename TypedAccessor<TreeT>::Ptr accessor(new TypedAccessor<TreeT>());
            mVoidAccessors.emplace_back(accessor->init(tree));
            mAccessors.emplace_back(std::move(accessor));
        }

        template <typename TreeT>
        inline void
        addConstAccessor(const TreeT& tree)
        {
            typename TypedAccessor<const TreeT>::Ptr accessor(new TypedAccessor<const TreeT>());
            mVoidAccessors.emplace_back(accessor->init(tree));
            mAccessors.emplace_back(std::move(accessor));
        }

        inline void
        addTransform(math::Transform::Ptr transform)
        {
            mVoidTransforms.emplace_back(static_cast<void*>(transform.get()));
        }

        const CustomData* const mCustomDataPtr;
        openvdb::Coord mCoord;
        openvdb::math::Vec3<float> mCoordWS;

    private:
        std::vector<void*> mVoidAccessors;
        std::vector<Accessors::Ptr> mAccessors;
        std::vector<void*> mVoidTransforms;
    };
};


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/// @brief Visitor object which will generate llvm IR for a syntax tree which has been generated
///        from AX that targets volumes.  The IR will represent a single function. It is mainly
///        used by the Compiler class.
struct VolumeComputeGenerator : public ComputeGenerator
{
    /// @brief Constructor
    /// @param module           llvm Module for generating IR
    /// @param customData       Custom data to be referenced from compiled AX code
    /// @param options          Options for the function registry behaviour
    /// @param functionRegistry Function registry object which will be used when generating IR
    ///                         for function calls
    /// @param warnings         Vector which will hold compiler warnings.  If null, no warnings will
    ///                         be stored.
    /// @param functionName     Name of the generated IR function
    VolumeComputeGenerator(llvm::Module& module,
                           CustomData* const customData,
                           const FunctionOptions& options,
                           FunctionRegistry& functionRegistry,
                           std::vector<std::string>* const warnings = nullptr,
                           const std::string& functionName = ComputeVolumeFunction::DefaultName);

    ~VolumeComputeGenerator() override = default;

    /// @brief Retrieve the name of the generated IR function
    /// @param list Vector of strings into which the name should be retrieved
    inline void
    getFunctionList(std::vector<std::string>& list)
    {
        list.push_back(mFunctionName);
    }

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
    size_t mVolumeVisitCount;
    std::string mFunctionName;
};

}
}
}
}

#endif // OPENVDB_AX_VOLUME_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
