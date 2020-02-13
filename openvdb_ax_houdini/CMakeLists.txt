# Copyright (c) 2015-2020 DNEG
#
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
#
# Redistributions of source code must retain the above copyright
# and license notice and the following restrictions and disclaimer.
#
# *     Neither the name of DNEG nor the names of
# its contributors may be used to endorse or promote products derived
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

cmake_minimum_required(VERSION 3.3)
project(OpenVDBAXHoudini LANGUAGES CXX)

option(USE_DEFAULT_HOUDINI_INSTALL [=[
Use the default install location returned by the Houdini CMake function,
houdini_get_default_install_dir():
  Linux: $ENV{HOME}/houdiniX.X
  Mac: $ENV{HOME}/Library/Preferences/houdini/X.X
  Windows: $ENV{HOMEDRIVE}$ENV{HOMEPATH}\Documents\houdiniX.X
If OFF, CMAKE_INSTALL_PREFIX is used.]=] ON)

#########################################################################

message(STATUS "----------------------------------------------------")
message(STATUS "---------- Configuring OpenVDB_AX_Houdini ----------")
message(STATUS "----------------------------------------------------")

#########################################################################

# Configure prefix

if(OPENVDB_AX_HOUDINI_INSTALL_PREFIX)
  message(DEPRECATION "OPENVDB_AX_HOUDINI_INSTALL_PREFIX option is deprecated")
  set(HOUDINI_INSTALL_LOCATION OPENVDB_AX_HOUDINI_INSTALL_PREFIX)
endif()

if(NOT OPENVDB_AX_HOUDINI_INSTALL_PREFIX)
  if(USE_DEFAULT_HOUDINI_INSTALL)
    # Function from the Houdini CMake which is brought in by OpenVDBHoudiniSetup.cmake
    houdini_get_default_install_dir(HOUDINI_INSTALL_LOCATION)
  else()
    set(HOUDINI_INSTALL_LOCATION ${CMAKE_INSTALL_PREFIX})
  endif()
endif()

set(OPENVDB_AX_HOUDINI_LIB_INSTALL_PREFIX ${HOUDINI_INSTALL_LOCATION}
  CACHE PATH "Base install path for OpenVDB Houdini shared libraries and dsos."
)
set(OPENVDB_AX_HOUDINI_INCLUDE_INSTALL_PREFIX ${HOUDINI_INSTALL_LOCATION}
  CACHE PATH "Base install path for OpenVDB Houdini headers."
)

message(STATUS "Houdini base library install path: ${OPENVDB_AX_HOUDINI_LIB_INSTALL_PREFIX}")
message(STATUS "Houdini base include install path: ${OPENVDB_AX_HOUDINI_INCLUDE_INSTALL_PREFIX}")

#########################################################################

# Get OpenVDB AX Core library

if(NOT OPENVDB_BUILD_AX)
  find_package(OpenVDB REQUIRED openvdb_ax)
  set(OPENVDBAX_LIB OpenVDB::openvdb_ax)
else ()
  set(OPENVDBAX_LIB openvdb_ax)
endif()

#### ADD TARGET

add_library(SOP_OpenVDB_AX SHARED SOP_OpenVDB_AX.cc)
set_target_properties(SOP_OpenVDB_AX
  PROPERTIES PREFIX ""
)

target_include_directories(SOP_OpenVDB_AX PRIVATE . )
target_link_libraries(SOP_OpenVDB_AX PUBLIC
  OpenVDB::openvdb_houdini
  ${OPENVDBAX_LIB}
  Houdini
)

if(OPENVDB_ENABLE_RPATH)
  set(RPATHS)
  if(OPENVDB_BUILD_AX)
    list(APPEND RPATHS ${CMAKE_INSTALL_PREFIX}/lib)
  else()
    list(APPEND RPATHS ${OpenVDB_LIBRARY_DIRS})
  endif()

  list(REMOVE_DUPLICATES RPATHS)
  set_target_properties(SOP_OpenVDB_AX
    PROPERTIES INSTALL_RPATH "${RPATHS}"
  )
endif()

##### INSTALL

set(OPENVDB_AX_HOUDINI_INCLUDE_FILES ax/HoudiniAXUtils.h)

install(TARGETS SOP_OpenVDB_AX
  DESTINATION
    ${OPENVDB_AX_HOUDINI_LIB_INSTALL_PREFIX}/dso
)

install(FILES ${OPENVDB_AX_HOUDINI_INCLUDE_FILES}
  DESTINATION
    ${OPENVDB_AX_HOUDINI_INCLUDE_INSTALL_PREFIX}/include/openvdb_ax_houdini/ax
)
