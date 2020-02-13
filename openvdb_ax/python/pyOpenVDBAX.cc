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

/// @file python/pyOpenVDBAX.cc
///
/// @authors Alexandros Gouvatsos, Richard Jones
///
/// @brief Python module for OpenVDB AX
///

#include "pyCompiler.h"
#include "pyPointExecutable.h"
#include "pyVolumeExecutable.h"

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/PrintTree.h>

#include <openvdb/openvdb.h>

#include <boost/python.hpp>

namespace {

/// @brief Print the Abstract Syntax Tree that is generated
///        by a supplied code snippet
/// @param code String AX code snippet
///
/// @todo Move this function into PrintTree.h
///
void printASTFromCode(const std::string& code)
{
    openvdb::ax::ast::Tree::Ptr
        syntaxTree = openvdb::ax::ast::parse(code.c_str());
    openvdb::ax::ast::print(*syntaxTree);
}

} // namespace anon

BOOST_PYTHON_MODULE(pyopenvdbax)
{
    openvdb::initialize();
    openvdb::ax::initialize();

    boost::python::scope().attr("__doc__") =
        "OpenVDB AX Python bindings for compiling and executing AX on "
        "pyopenvdb Grid objects.";

    pyopenvdb::ax::exportPointExecutable();
    pyopenvdb::ax::exportVolumeExecutable();
    pyopenvdb::ax::exportCompiler();

    boost::python::def("printAST",
        &printASTFromCode,
        boost::python::arg("code"),
        "printAST(code)\n\n"
        "Print the Abstract Syntax Tree created from the code supplied.\n");
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
