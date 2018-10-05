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

/// @file codegen/ComputeGenerator.h
///
/// @authors Nick Avramoussis, Matt Warner, Francisco Gochez, Richard Jones
///
/// @brief  The core visitor framework for code generation
///

#ifndef OPENVDB_AX_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

#include "FunctionRegistry.h"
#include "FunctionTypes.h"

#include "SymbolTable.h"

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/compiler/CompilerOptions.h>

#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <map>
#include <stack>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

/// @brief  The function definition and signature which is built by the
///         ComputeGenerator.
///
///         The argument structure is as follows:
///
///           1) - A void pointer to the CustomData
///
struct ComputeKernel
{
    /// The name of the generated function
    static const std::string Name;

    /// The signature of the generated function
    using Signature = void(const void* const);

    using FunctionT = std::function<Signature>;
    using FunctionTraitsT = codegen::FunctionTraits<FunctionT>;
    static const size_t N_ARGS = FunctionTraitsT::N_ARGS;

    /// The argument key names available during code generation
    static const std::array<std::string, N_ARGS>& getArgumentKeys();
    static std::string getDefaultName();
};


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


struct ComputeGenerator : public ast::Visitor
{

    ComputeGenerator(llvm::Module& module,
                     const FunctionOptions& options,
                     FunctionRegistry& functionRegistry,
                     std::vector<std::string>* const warnings = nullptr);

    ~ComputeGenerator() override = default;

    inline SymbolTable& globals() { return mSymbolTables.globals(); }
    inline const SymbolTable& globals() const { return mSymbolTables.globals(); }

protected:

    // The following methods are typically overridden based on the volume
    // access

    void init(const ast::Tree& node) override;
    void visit(const ast::AssignExpression& node) override;
    void visit(const ast::Crement& node) override;
    void visit(const ast::FunctionCall& node) override;
    void visit(const ast::Attribute& node) override;
    void visit(const ast::AttributeValue& node) override;
    ////

    void visit(const ast::Tree& node) override;
    void visit(const ast::Block& node) override;
    void visit(const ast::ConditionalStatement& node) override;
    void visit(const ast::Return& node) override;
    void visit(const ast::UnaryOperator& node) override;
    void visit(const ast::BinaryOperator& node) override;
    void visit(const ast::Cast& node) override;
    void visit(const ast::DeclareLocal& node) override;
    void visit(const ast::Local& node) override;

    /// @brief VectorUnpack pushes a single llvm::Value onto the value
    /// stack which represents a pointer to an element of a vector
    ///
    void visit(const ast::VectorUnpack& node) override;

    /// @brief VectorPack pushes a single llvm::Value onto the value
    /// stack which represents a pointer to a vector
    ///
    void visit(const ast::VectorPack& node) override;
    void visit(const ast::ArrayPack& node) override;

    /// @brief Value<> scalar values and push a single llvm::Value
    /// onto the value stack which represents a pointer to a scalar
    ///
    void visit(const ast::Value<bool>& node) override;
    void visit(const ast::Value<int16_t>& node) override;
    void visit(const ast::Value<int32_t>& node) override;
    void visit(const ast::Value<int64_t>& node) override;
    void visit(const ast::Value<float>& node) override;
    void visit(const ast::Value<double>& node) override;
    void visit(const ast::Value<std::string>& node) override;

protected:

    FunctionBase::Ptr getFunction(const std::string& identifier, const FunctionOptions& op, const bool allowInternal = false);

    llvm::Module& mModule;
    llvm::LLVMContext& mContext;
    llvm::IRBuilder<> mBuilder;

    // Holds all scoped blocks, including the initial insert point
    std::stack<llvm::BasicBlock*> mBlocks;
    std::vector<llvm::BasicBlock*> mReturnBlocks;

    // Used to hold break points (post conditional statement) for exiting blocks
    // @TODO add support for break keyword
    std::stack<llvm::BasicBlock*> mContinueBlocks;

    // The current block number used to track scoped declarations
    size_t mCurrentBlock;

    // The stack of accessed values
    std::stack<llvm::Value*> mValues;

    // The map of block number to local variable names to values
    SymbolTableBlocks mSymbolTables;

    // Warnings that are generated during code generation
    std::vector<std::string>* const mWarnings;

    // The function used as the base code block
    llvm::Function* mFunction;

    // The string mapped function variables, defined by the Function interface
    SymbolTable mLLVMArguments;

    const FunctionOptions mOptions;

private:

    template <typename ValueType>
    typename std::enable_if<std::is_integral<ValueType>::value>::type
    visit(const ast::Value<ValueType>& node);

    template <typename ValueType>
    typename std::enable_if<std::is_floating_point<ValueType>::value>::type
    visit(const ast::Value<ValueType>& node);

    const std::unique_ptr<const llvm::TargetLibraryInfoImpl> mTargetLibInfoImpl;
    FunctionRegistry& mFunctionRegistry;
};

}
}
}
}

#endif // OPENVDB_AX_COMPUTE_GENERATOR_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
