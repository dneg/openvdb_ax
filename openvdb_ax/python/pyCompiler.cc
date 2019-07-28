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

#include "pyCompiler.h"

namespace pyopenvdb {
namespace ax {

CompilerWrap::CompilerWrap()
    : mCompiler(openvdb::ax::Compiler::create()) {}

PointExecutableWrap CompilerWrap::compilePoints(const std::string& code) {
    openvdb::ax::PointExecutable::Ptr
        pointExecutable = mCompiler->compile<openvdb::ax::PointExecutable>(code);
    return PointExecutableWrap(pointExecutable);
}

VolumeExecutableWrap CompilerWrap::compileVolumes(const std::string& code) {
    openvdb::ax::VolumeExecutable::Ptr
        volumeExecutable = mCompiler->compile<openvdb::ax::VolumeExecutable>(code);
    return VolumeExecutableWrap(volumeExecutable);
}

void exportCompiler() {

    boost::python::class_<pyopenvdb::ax::CompilerWrap>("Compiler")
        .def("compilePoints", &pyopenvdb::ax::CompilerWrap::compilePoints)
        .def("compileVolumes", &pyopenvdb::ax::CompilerWrap::compileVolumes);
}

} // namespace ax
} // namespace pyopenvdb

// Copyright (c) 2015-2019 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
