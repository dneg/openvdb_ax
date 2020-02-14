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

/// @file python/pyPointExecutable.h
///
/// @authors Alexandros Gouvatsos, Richard Jones
///
/// @brief Python wrapper for PointExecutable
///

#ifndef PY_POINTEXECUTABLE_HAS_BEEN_INCLUDED
#define PY_POINTEXECUTABLE_HAS_BEEN_INCLUDED

#include <openvdb_ax/compiler/PointExecutable.h>

#include <boost/python.hpp>

namespace pyopenvdb {
namespace ax {

class PointExecutableWrap {

public:
    PointExecutableWrap(openvdb::ax::PointExecutable::Ptr pointExecutable);

    /// @brief Execution of the PointExecutable over a supplied
    ///        Points VDB in Python
    /// @param gridObj a grid object
    ///
    void execute(const boost::python::object& gridObj);

    /// @brief Execution of the PointExecutable over a supplied
    ///        list of Points VDBs in Python
    /// @param gridObjs a list of grid objects
    ///
    void execute(const boost::python::list& gridObjs);

private:
    openvdb::ax::PointExecutable::Ptr mPointExecutable;
};

void exportPointExecutable();

} // namespace ax
} // namespace pyopenvdb

#endif // PY_POINTEXECUTABLE_HAS_BEEN_INCLUDED

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
