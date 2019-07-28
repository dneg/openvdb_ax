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
// *     Neither the name of DNEG nor the names of
// its contributors may be used to endorse or promote products derived
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

#include "FunctionRegistry.h"
#include "Functions.h"
#include "FunctionTypes.h"

#include <openvdb_ax/Exceptions.h>

namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {

namespace ax {
namespace codegen {

void FunctionRegistry::insert(const std::string& identifier,
       const FunctionRegistry::ConstructorT creator, const bool internal)
{
    if (!mMap.emplace(std::piecewise_construct,
              std::forward_as_tuple(identifier),
              std::forward_as_tuple(creator, internal)).second) {
        OPENVDB_THROW(LLVMFunctionError, "A function already exists"
            " with the provided identifier: \"" + identifier + "\"");
    }
}

void FunctionRegistry::insertAndCreate(const std::string& identifier,
                const FunctionRegistry::ConstructorT creator,
                const FunctionOptions& op,
                const bool internal)
{
    auto inserted = mMap.emplace(std::piecewise_construct,
              std::forward_as_tuple(identifier),
              std::forward_as_tuple(creator, internal));
    if (!inserted.second) {
        OPENVDB_THROW(LLVMFunctionError, "A function already exists"
            " with the provided token: \"" + identifier + "\"");
    }
    inserted.first->second.create(op);
}

FunctionBase::Ptr FunctionRegistry::getOrInsert(const std::string& identifier,
                                                const FunctionOptions& op,
                                                const bool allowInternalAccess)
{
    auto iter = mMap.find(identifier);
    if (iter == mMap.end()) return FunctionBase::Ptr();
    FunctionRegistry::RegisteredFunction& reg = iter->second;
    if (!allowInternalAccess && reg.isInternal()) return FunctionBase::Ptr();

    if (!reg.function()) reg.create(op);

    FunctionBase::Ptr function = reg.function();

    // initialize function dependencies if necessary

    if (op.mLazyFunctions && function) {
        std::vector<std::string> dependencies;
        function->getDependencies(dependencies);
        for (const auto& dep : dependencies) {
            // if the function ptr doesn't exist, create it with getOrInsert.
            // This provides internal access and ensures handling of cyclical
            // dependencies do not cause a problem
            FunctionBase::Ptr internal = this->get(dep, true);
            if (!internal) this->getOrInsert(dep, op, true);
        }
    }

    return function;
}

FunctionBase::Ptr FunctionRegistry::get(const std::string& identifier, const bool allowInternalAccess) const
{
    auto iter = mMap.find(identifier);
    if (iter == mMap.end()) return FunctionBase::Ptr();
    if (!allowInternalAccess && iter->second.isInternal()) return FunctionBase::Ptr();
    return iter->second.function();
}

void FunctionRegistry::createAll(const FunctionOptions& op)
{
    for (auto& it : mMap) it.second.create(op);
}

}
}
}
}

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
