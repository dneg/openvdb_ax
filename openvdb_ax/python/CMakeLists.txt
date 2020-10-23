# Copyright (c) 2015-2020 DNEG
#
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
#
# Redistributions of source code must retain the above copyright
# and license notice and the following restrictions and disclaimer.
#
# *     Neither the name of DNEG nor the names
# of its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
# LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
#
#[=======================================================================[

  CMake Configuration for the OpenVDB AX Houdini Plugin.

#]=======================================================================]

cmake_minimum_required(VERSION 3.12)
project(OpenVDBAXPython LANGUAGES CXX)

#########################################################################

message(STATUS "----------------------------------------------------")
message(STATUS "----------- Configuring OpenVDBAXPython ------------")
message(STATUS "----------------------------------------------------")

##########################################################################

if(NOT OPENVDB_BUILD_AX)
  set(OPENVDBAX_LIB OpenVDB::openvdb_ax)
else()
  set(OPENVDBAX_LIB openvdb_ax)
endif()

set(PYOPENVDBAX_SRC_FILES
  pyCompiler.cc
  pyPointExecutable.cc
  pyVolumeExecutable.cc
  pyOpenVDBAX.cc
)

add_library(pyopenvdbax
  SHARED
  ${PYOPENVDBAX_SRC_FILES}
)

target_link_libraries(pyopenvdbax
  OpenVDB::pyopenvdb
  ${OPENVDBAX_LIB}
)

set_target_properties(pyopenvdbax PROPERTIES
  PREFIX ""  # no 'lib' prefix
)

if(UNIX)
  # must be .so (not .dylib)
  set_target_properties(pyopenvdbax PROPERTIES
    SUFFIX ".so"
  )
endif()

install(TARGETS pyopenvdbax
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
)
