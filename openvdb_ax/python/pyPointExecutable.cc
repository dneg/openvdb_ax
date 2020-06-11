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

/// @file python/pyPointExecutable.cc

#include "pyPointExecutable.h"

#include <openvdb/openvdb.h>
#include <openvdb/python/pyopenvdb.h>
#include <openvdb/points/PointDataGrid.h>

namespace pyopenvdb {
namespace ax {

PointExecutableWrap::PointExecutableWrap(openvdb::ax::PointExecutable::Ptr pointExecutable)
    : mPointExecutable(pointExecutable) {}

void PointExecutableWrap::execute(const boost::python::object& gridObj) {
    openvdb::GridBase::Ptr grid = pyopenvdb::getGridFromPyObject(gridObj);
    openvdb::points::PointDataGrid::Ptr points =
            openvdb::gridPtrCast<openvdb::points::PointDataGrid>(grid);
    if (!points) return;
    mPointExecutable->execute(*points);
}

void PointExecutableWrap::execute(const boost::python::list& gridObjs) {
    const boost::python::ssize_t numGrids = boost::python::len(gridObjs);

    for (boost::python::ssize_t i = 0; i < numGrids; i++) {
        const boost::python::object& gridObj = gridObjs[i];
        openvdb::GridBase::Ptr grid = pyopenvdb::getGridFromPyObject(gridObj);
        openvdb::points::PointDataGrid::Ptr points =
                openvdb::gridPtrCast<openvdb::points::PointDataGrid>(grid);
        if (points) mPointExecutable->execute(*points);
    }
}

void exportPointExecutable() {

    boost::python::class_<PointExecutableWrap>("PointExecutable", boost::python::no_init)
        .def<void (PointExecutableWrap::*)
            (const boost::python::object&)>("execute", &PointExecutableWrap::execute)
        .def<void (PointExecutableWrap::*)
            (const boost::python::list&)>("execute", &PointExecutableWrap::execute);
}

} // namespace ax
} // namespace pyopenvdb

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
