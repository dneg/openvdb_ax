# Copyright (c) 2015-2019 DNEG
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
cmake_minimum_required( VERSION 3.1 )
project ( OpenVDB_AX )

enable_testing()

find_program ( CCACHE_FOUND ccache )
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

option ( OPENVDB_AX_BUILD_AX "Build the OpenVDB AX compiler" ON )
option ( OPENVDB_AX_BUILD_UNITTESTS "Build the OpenVDB AX unit tests" OFF )
option ( OPENVDB_AX_BUILD_DOCS "Build the OpenVDB AX documentation" OFF )
option ( OPENVDB_AX_ENABLE_RPATH "Build with RPATH information" ON )
option ( OPENVDB_AX_DISABLE_BOOST_IMPLICIT_LINKING "Disable the implicit linking of Boost libraries on Windows" ON )
option ( OPENVDB_AX_BUILD_HOUDINI_SOP "Build the OpenVDB AX Houdini SOP" OFF )

list ( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )

set ( OPENVDB_AX_TOP_LEVEL_DIR ${PROJECT_SOURCE_DIR} CACHE PATH "OpenVDB AX Top Level Directory to share among sub projects" )

include_directories ( ${OPENVDB_AX_TOP_LEVEL_DIR} ) # To make sure we pick up headers from this version of OpenVDB AX we are building

# Check ABI has been provided

IF ( ${OPENVDB_ABI_VERSION_NUMBER} )
  MESSAGE ( STATUS "OpenVDB ABI version: ${OPENVDB_ABI_VERSION_NUMBER}")
  add_definitions (-DOPENVDB_ABI_VERSION_NUMBER=${OPENVDB_ABI_VERSION_NUMBER} )
ELSE ()
    MESSAGE ( FATAL_ERROR "An OpenVDB ABI Version must be provided.\n"
        "e.g. -D OPENVDB_ABI_VERSION_NUMBER=3" )
ENDIF ()

if ( OPENVDB_AX_BUILD_AX )
  ADD_SUBDIRECTORY ( openvdb_ax )
endif ()

if ( OPENVDB_AX_BUILD_HOUDINI_SOP )
  ADD_SUBDIRECTORY ( openvdb_ax_houdini )
endif ()
