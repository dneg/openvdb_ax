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

/// @file compiler/Compiler.h
///
/// @authors Nick Avramoussis, Francisco Gochez, Richard Jones
///
/// @brief  The OpenVDB AX Compiler class provides methods to generate
///   AX executables from a provided AX AST (or directly from a given
///   string). The class object exists to cache various structures,
///   primarily LLVM constructs, which benefit from existing across
///   additional compilation runs.
///

#ifndef OPENVDB_AX_COMPILER_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPILER_HAS_BEEN_INCLUDED

#include <openvdb/version.h>

#include "../ax.h" // backward compat support for initialize()
#include "../ast/Parse.h"
#include "../compiler/CompilerOptions.h"
#include "../compiler/CustomData.h"
#include "../compiler/Logger.h"

#include <memory>
#include <sstream>

// forward
namespace llvm {
class LLVMContext;
}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {

namespace codegen {
// forward
class FunctionRegistry;
}

/// @brief  The compiler class.  This holds an llvm context and set of compiler
///   options, and constructs executable objects (e.g. PointExecutable or
///   VolumeExecutable) from a syntax tree or snippet of code.
class Compiler
{
public:

    using Ptr = std::shared_ptr<Compiler>;
    using UniquePtr = std::unique_ptr<Compiler>;

    /// @brief Construct a compiler object with given settings
    /// @param options CompilerOptions object with various settings
    Compiler(const CompilerOptions& options = CompilerOptions());

    ~Compiler() = default;

    /// @brief Static method for creating Compiler objects
    static UniquePtr create(const CompilerOptions& options = CompilerOptions());

    /// @brief Compile a given AST into an executable object of the given type.
    /// @param syntaxTree An abstract syntax tree to compile
    /// @param logger Logger for errors and warnings during compilation, this
    ///   should be linked to an ast::Tree and populated with AST node + line
    ///   number mappings for this Tree, e.g. during ast::parse(). This Tree can
    ///   be different from the syntaxTree argument.
    /// @param data Optional external/custom data which is to be referenced by
    ///   the executable object. It allows one to reference data held elsewhere,
    ///   such as inside of a DCC, from inside the AX code
    /// @note If the logger has not been populated with AST node and line
    ///   mappings, all messages will appear without valid line and column
    ///   numbers.
    template <typename ExecutableT>
    typename ExecutableT::Ptr
    compile(const ast::Tree& syntaxTree,
            Logger& logger,
            const CustomData::Ptr data = CustomData::Ptr());

    /// @brief Compile a given snippet of AX code into an executable object of
    ///   the given type.
    /// @param code A string of AX code
    /// @param logger Logger for errors and warnings during compilation, will be
    ///   cleared of existing data
    /// @param data Optional external/custom data which is to be referenced by
    ///   the executable object. It allows one to reference data held elsewhere,
    ///   such as inside of a DCC, from inside the AX code
    /// @note  If compilation is unsuccessful, will return nullptr. Logger can
    ///   then be queried for errors.
    template <typename ExecutableT>
    typename ExecutableT::Ptr
    compile(const std::string& code,
            Logger& logger,
            const CustomData::Ptr data = CustomData::Ptr())
    {
        logger.clear();
        const ast::Tree::ConstPtr syntaxTree = ast::parse(code.c_str(), logger);
        if (syntaxTree) return compile<ExecutableT>(*syntaxTree, logger, data);
        else return nullptr;
    }

    /// @brief Compile a given snippet of AX code into an executable object of
    ///   the given type.
    /// @param code A string of AX code
    /// @param data Optional external/custom data which is to be referenced by
    ///   the executable object. It allows one to reference data held elsewhere,
    ///   such as inside of a DCC, from inside the AX code
    /// @note Parser errors are handled separately from compiler errors.
    ///   Each are collected and produce runtime errors.
    template <typename ExecutableT>
    typename ExecutableT::Ptr
    compile(const std::string& code,
            const CustomData::Ptr data = CustomData::Ptr())
    {
        std::vector<std::string> errors;
        openvdb::ax::Logger logger(
            [&errors] (const std::string& error) {
                errors.emplace_back(error + "\n");
            },
            // ignore warnings
            [] (const std::string&) {}
        );
        const ast::Tree::ConstPtr syntaxTree = ast::parse(code.c_str(), logger);
        typename ExecutableT::Ptr exe;
        if (syntaxTree) {
            exe = this->compile<ExecutableT>(*syntaxTree, logger, data);
        }
        if (!errors.empty()) {
            std::ostringstream os;
            for (const auto& e : errors) os << e << "\n";
            OPENVDB_THROW(AXCompilerError, os.str());
        }
        assert(exe);
        return exe;
    }

    /// @brief Compile a given AST into an executable object of the given type.
    /// @param syntaxTree An abstract syntax tree to compile
    /// @param data Optional external/custom data which is to be referenced by
    ///   the executable object. It allows one to reference data held elsewhere,
    ///   such as inside of a DCC, from inside the AX code
    /// @note Any errors encountered are collected into a single runtime error
    template <typename ExecutableT>
    typename ExecutableT::Ptr
    compile(const ast::Tree& syntaxTree,
            const CustomData::Ptr data = CustomData::Ptr())
    {
        std::vector<std::string> errors;
        openvdb::ax::Logger logger(
            [&errors] (const std::string& error) {
                errors.emplace_back(error + "\n");
            },
            // ignore warnings
            [] (const std::string&) {}
        );
        auto exe = compile<ExecutableT>(syntaxTree, logger, data);
        if (!errors.empty()) {
            std::ostringstream os;
            for (const auto& e : errors) os << e << "\n";
            OPENVDB_THROW(AXCompilerError, os.str());
        }
        assert(exe);
        return exe;
    }

    /// @brief Sets the compiler's function registry object.
    /// @param functionRegistry A unique pointer to a FunctionRegistry object.
    ///   The compiler will take ownership of the registry that was passed in.
    /// @todo  Perhaps allow one to register individual functions into this
    ///   class rather than the entire registry at once, and/or allow one to
    ///   extract a pointer to the registry and update it manually.
    void setFunctionRegistry(std::unique_ptr<codegen::FunctionRegistry>&& functionRegistry);

    ///////////////////////////////////////////////////////////////////////////

    /// @brief deprecated methods
    template <typename ExecutableT>
    OPENVDB_DEPRECATED
    typename ExecutableT::Ptr
    compile(const ast::Tree& syntaxTree,
            const CustomData::Ptr data,
            std::vector<std::string>* warnings) {
        openvdb::ax::Logger logger(
            // throw immediately on first error
            [] (const std::string& error) {
            OPENVDB_THROW(AXSyntaxError, error);
            },
            // collect warnings in vector
            [&warnings] (const std::string& warn) {
                if (warnings) warnings->emplace_back(warn);
            }
        );
        return compile<ExecutableT>(syntaxTree, logger, data);
    }

    template <typename ExecutableT>
    OPENVDB_DEPRECATED
    typename ExecutableT::Ptr
    compile(const std::string& code,
            const CustomData::Ptr data,
            std::vector<std::string>* warnings) {
        openvdb::ax::Logger logger(
            // throw immediately on first error
            [] (const std::string& error) {
            OPENVDB_THROW(AXSyntaxError, error);
            },
            // collect warnings in vector
            [&warnings] (const std::string& warn) {
                if (warnings) warnings->emplace_back(warn);
            }
        );
        return compile<ExecutableT>(code, logger, data);
    }

    ///////////////////////////////////////////////////////////////////////////

private:

    std::shared_ptr<llvm::LLVMContext> mContext;
    const CompilerOptions mCompilerOptions;
    std::shared_ptr<codegen::FunctionRegistry> mFunctionRegistry;
};


}
}
}

#endif // OPENVDB_AX_COMPILER_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
