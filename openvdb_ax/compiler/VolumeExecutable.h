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

/// @file compiler/VolumeExecutable.h
///
/// @authors Nick Avramoussis, Francisco Gochez
///
/// @brief Contains VolumeExecutable class
///

#ifndef OPENVDB_AX_COMPILER_VOLUME_EXECUTABLE_HAS_BEEN_INCLUDED
#define OPENVDB_AX_COMPILER_VOLUME_EXECUTABLE_HAS_BEEN_INCLUDED

#include <openvdb_ax/compiler/CustomData.h>
#include <openvdb_ax/compiler/TargetRegistry.h>

#include <openvdb/openvdb.h>

//forward
namespace llvm {

    class ExecutionEngine;
    class LLVMContext;

}

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {

/// @brief Object that encapsulates compiled AX code which can be executed on a collection of
///        VDB volume grids
class VolumeExecutable
{
public:
    using Ptr = std::shared_ptr<VolumeExecutable>;
    using Registry = VolumeRegistry;

    /// @brief Constructor
    /// @param exeEngine Shared pointer to an llvm::ExecutionEngine object used to build functions.
    ///        context should be the associated llvm context
    /// @param context Shared pointer to an llvm:context object associated with exeEngine
    /// @param volumeRegistry Registry of volumes accessed by AX code
    /// @param customData Custom data object which will be shared by this executable.  It can be
    ///        used to retrieve external data from within the AX code
    /// @param functionAddresses A Vector of maps of function names to physical memory addresses which were built
    ///        by llvm using exeEngine
    /// @param assignedVolumes Vector of names of volumes which are written to, in order.
    /// @note  This object is normally be constructed by the Compiler::compile method, rather
    ///        than directly
    VolumeExecutable(const std::shared_ptr<const llvm::ExecutionEngine>& exeEngine,
                     const std::shared_ptr<const llvm::LLVMContext>& context,
                     const VolumeRegistry::ConstPtr& volumeRegistry,
                     const CustomData::Ptr& customData,
                     const std::vector<std::map<std::string, uint64_t> >& functionAddresses,
                     const std::vector<std::string>& assignedVolumes)
        : mExecutionEngine(exeEngine)
        , mContext(context)
        , mVolumeRegistry(volumeRegistry)
        , mCustomData(customData)
        , mBlockFunctionAddresses(functionAddresses)
        , mAssignedVolumes(assignedVolumes) {}

    ~VolumeExecutable() = default;

    /// @brief Execute AX code on target grids
    void execute(const openvdb::GridPtrVec& grids) const;

private:

    // these 2 shared pointers exist _only_ for object lifetime management
    // as these objects must not be destroyed before this one
    const std::shared_ptr<const llvm::ExecutionEngine> mExecutionEngine;
    const std::shared_ptr<const llvm::LLVMContext> mContext;

    const Registry::ConstPtr mVolumeRegistry;

    const CustomData::Ptr mCustomData;
    const std::vector<std::map<std::string, uint64_t> > mBlockFunctionAddresses;
    const std::vector<std::string> mAssignedVolumes;
};

}
}
}

#endif // OPENVDB_AX_COMPILER_VOLUME_EXECUTABLE_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
