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

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

/// @brief  The function definition and signature which is built by the
///         VolumeComputeGenerator.
///
///         The argument structure is as follows:
///
///             1) - A void pointer to the CustomData
///             2) - A pointer to an array of three ints representing the
///                  current voxel coord being accessed
///             3) - An pointer to an array of three floats representing the
///                  current voxel world space coord being accessed
///             4) - A void pointer to a vector of void pointers, representing
///                  an array of grid accessors
///             5) - A void pointer to a vector of void pointers, representing
///                  an array of grid transforms
///
struct VolumeKernel
{
    // The signature of the generated function
    using Signature =
        void(const void* const,
             const int32_t (*)[3],
             const float (*)[3],
             void**,
             void**);

    using FunctionT = std::function<Signature>;
    using FunctionTraitsT = codegen::FunctionTraits<FunctionT>;
    static const size_t N_ARGS = FunctionTraitsT::N_ARGS;

    static const std::array<std::string, N_ARGS>& argumentKeys();
    static std::string getDefaultName();
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

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
                           const FunctionOptions& options,
                           FunctionRegistry& functionRegistry,
                           std::vector<std::string>* const warnings = nullptr);

    ~VolumeComputeGenerator() override = default;

    void setFunctionName(const std::string& name);

protected:

    /// @brief initializes visitor.  Automatically called when visiting the tree's root node.
    void init(const ast::Tree& node) override;
    void visit(const ast::AssignExpression& node) override;
    void visit(const ast::Crement& node) override;
    void visit(const ast::FunctionCall& node) override;
    void visit(const ast::Attribute& node) override;
    void visit(const ast::AttributeValue& node) override;

private:

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
