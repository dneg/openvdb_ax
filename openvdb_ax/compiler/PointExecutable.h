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

/// @file compiler/PointExecutable.h
///
/// @authors Nick Avramoussis, Francisco Gochez, Richard Jones
///
/// @brief The PointExecutable, produced by the OpenVDB AX Compiler for
///   execution over OpenVDB Points Grids.
///

#ifndef OPENVDB_AX_COMPILER_POINT_EXECUTABLE_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPILER_POINT_EXECUTABLE_HAS_BEEN_INCLUDED

#include <openvdb/openvdb.h>
#include <openvdb/version.h>
#include <openvdb/points/PointDataGrid.h>

#include "../compiler/CustomData.h"
#include "../compiler/AttributeRegistry.h"

#include <unordered_map>

class TestPointExecutable;

namespace llvm {
class ExecutionEngine;
class LLVMContext;
}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {
namespace ax {

class Compiler;

/// @brief Object that encapsulates compiled AX code which can be executed on a target point grid
class PointExecutable
{
public:
    using Ptr = std::shared_ptr<PointExecutable>;
    ~PointExecutable();

    /// @brief  Copy constructor. Shares the LLVM constructs but deep copies the
    ///   settings. Multiple copies of an executor can be used at the same time
    ///   safely.
    PointExecutable(const PointExecutable& other);

    ////////////////////////////////////////////////////////

    /// @brief executes compiled AX code on target grid
    void execute(points::PointDataGrid& grid) const;

    ////////////////////////////////////////////////////////

    /// @brief  Set a specific point group to execute over. The default is none,
    ///   which corresponds to all points. Note that this can also be compiled
    ///   into the AX function using the ingroup("mygroup") method.
    /// @warning  If the group does not exist during execute, a runtime error
    ///   will be thrown.
    /// @param name  The name of the group to execute over
    void setGroupExecution(const std::string& name);
    /// @return  The points group to be processed. Default is empty, which is
    ///   all points.
    const std::string& getGroupExecution() const;

    /// @brief  Set the behaviour when missing point attributes are accessed.
    ///    Default behaviour is true, which creates them with default initial
    ///    values. If false, a missing attribute runtime error will be thrown
    ///    on missing accesses.
    /// @param flag  Enables or disables the creation of missing attributes
    void setCreateMissing(const bool flag);
    /// @return  Whether this executable will generate new point attributes.
    bool getCreateMissing() const;

    /// @brief  Set the threading grain size. Default is 1. A value of 0 has the
    ///   effect of disabling multi-threading.
    /// @param grain The grain size
    void setGrainSize(const size_t grain);
    /// @return  The current grain size
    size_t getGrainSize() const;

    ////////////////////////////////////////////////////////

    // @brief deprecated methods
    OPENVDB_DEPRECATED void
    execute(points::PointDataGrid& grid,
        const std::string* const group,
        const bool create) const
    {
        PointExecutable copy(*this);
        if (group) copy.setGroupExecution(*group);
        copy.setCreateMissing(create);
        copy.execute(grid);
    }

    OPENVDB_DEPRECATED void
    execute(points::PointDataGrid& grid,
        const std::string* const group) const
    {
        PointExecutable copy(*this);
        if (group) copy.setGroupExecution(*group);
        copy.execute(grid);
    }

    ////////////////////////////////////////////////////////

    // foward declaration of settings for this executable
    struct Settings;

private:
    friend class Compiler;
    friend class ::TestPointExecutable;

    /// @brief Constructor, expected to be invoked by the compiler. Should not
    ///   be invoked directly.
    /// @param context Shared pointer to an llvm:LLVMContext associated with the
    ///   execution engine
    /// @param engine Shared pointer to an llvm::ExecutionEngine used to build
    ///   functions. Context should be the associated LLVMContext
    /// @param attributeRegistry Registry of attributes accessed by AX code
    /// @param customData Custom data which will be shared by this executable.
    ///   It can be used to retrieve external data from within the AX code
    /// @param functions A map of function names to physical memory addresses
    ///   which were built by llvm using engine
    PointExecutable(const std::shared_ptr<const llvm::LLVMContext>& context,
                    const std::shared_ptr<const llvm::ExecutionEngine>& engine,
                    const AttributeRegistry::ConstPtr& attributeRegistry,
                    const CustomData::ConstPtr& customData,
                    const std::unordered_map<std::string, uint64_t>& functions);

private:
    // The Context and ExecutionEngine must exist _only_ for object lifetime
    // management. The ExecutionEngine must be destroyed before the Context
    const std::shared_ptr<const llvm::LLVMContext> mContext;
    const std::shared_ptr<const llvm::ExecutionEngine> mExecutionEngine;
    const AttributeRegistry::ConstPtr mAttributeRegistry;
    const CustomData::ConstPtr mCustomData;
    const std::unordered_map<std::string, uint64_t> mFunctionAddresses;
    std::unique_ptr<Settings> mSettings;
};

}
}
}

#endif // OPENVDB_AX_COMPILER_POINT_EXECUTABLE_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
