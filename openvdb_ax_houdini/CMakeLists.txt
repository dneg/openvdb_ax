# Copyright (c) 2015-2018 DNEG Visual Effects
#
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
#
# Redistributions of source code must retain the above copyright
# and license notice and the following restrictions and disclaimer.
#
# *     Neither the name of DNEG Visual Effects nor the names of
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

cmake_minimum_required ( VERSION 3.1 )
set ( CMAKE_CXX_STANDARD 11 )

project ( OpenVDB_AX_Houdini )

##### SETUP DEPENDENCIES

# Get OpenVDB AX Core library

if ( NOT OPENVDB_AX_BUILD_AX )

  # If not building OpenVDB AX, assume it is already installed

  find_package ( OpenVDB_AX REQUIRED )
  set( OPENVDB_AX_SHARED_LIB ${OpenVDB_AX_LIBRARY} )
  set( OPENVDB_AX_INCLUDE_DIR ${OpenVDB_AX_INCLUDE_DIR})
else ()

  # If OpenVDB AX is being built as well, use the target set in openvdb_ax/CMakeLists.txt

  set ( OPENVDB_AX_SHARED_LIB openvdb_ax_shared )
  set ( OPENVDB_AX_INCLUDE_DIR ${OPENVDB_AX_TOP_LEVEL_DIR} )

endif ( NOT OPENVDB_AX_BUILD_AX )

find_package ( OpenVDB_Houdini REQUIRED )
find_package ( OpenVDB REQUIRED )

set( OPENVDB_HOUDINI_SHARED_LIB ${OPENVDB_HOUDINI_LIBRARY} )
set( OPENVDB_SHARED_LIB ${OpenVDB_OPENVDB_LIBRARY} )

find_package ( HDK REQUIRED )

if ( NOT WIN32 )
  add_definitions ( -pthread -fPIC )
endif ()

# RPath handling
if ( OPENVDB_AX_ENABLE_RPATH )

  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

  # Use $ORIGIN for relative referencing
  # The second $ORIGIN is present for the openvdb_houdini shared library.
  if( OPENVDB_HOUDINI_INSTALL_LIBRARY )
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../../lib:$ORIGIN/../lib")
  else ()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../../lib:$ORIGIN")
  endif ()
  # SET(CMAKE_INSTALL_RPATH "$ORIGIN")

endif ( OPENVDB_AX_ENABLE_RPATH )

#### ADD TARGET

hdk_add_library ( SOP_OpenVDB_AX
  SOP_OpenVDB_AX.cc
  )

##### INCLUDES AND LINKING

target_include_directories ( SOP_OpenVDB_AX
  PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${OPENVDB_AX_INCLUDE_DIR}
  ${OPENVDB_HOUDINI_INCLUDE_DIR}
  #${OPENVDB_HOUDINI_UTILS_INCLUDE_DIR} #this is the same as above
  )

target_include_directories ( SOP_OpenVDB_AX
  SYSTEM
  PUBLIC
  ${OpenVDB_INCLUDE_DIR}
)

target_include_directories ( SOP_OpenVDB_AX
  SYSTEM
  PUBLIC
  ${HDK_INCLUDE_DIR}
  )

# Checking if hdk version is >= 16.5
# Houdini 16.5 stopped shipping boost.

if (( HDK_VERSION_MAJOR GREATER 16 ) OR
    (( HDK_VERSION_MAJOR EQUAL 16 ) AND ( HDK_VERSION_MINOR EQUAL 5 )))

  find_package ( Boost ${MINIMUM_BOOST_VERSION} REQUIRED )

  target_include_directories ( SOP_OpenVDB_AX
    SYSTEM
    PUBLIC
    ${Boost_INCLUDE_DIR}
    )

endif()

link_directories( $ENV{HFS}/dsolib )

target_link_libraries ( SOP_OpenVDB_AX
  ${OPENVDB_AX_SHARED_LIB}
  ${OPENVDB_HOUDINI_SHARED_LIB}
  ${OPENVDB_SHARED_LIB}
  )

##### INSTALL

target_compile_options(SOP_OpenVDB_AX
  PRIVATE
  -fext-numeric-literals
  )

install ( TARGETS SOP_OpenVDB_AX
  DESTINATION lib/houdini/dso
  )

set ( OPENVDB_AX_HOUDINI_INCLUDE_FILES
  ax/HoudiniAXUtils.h
  )

install (
  FILES ${OPENVDB_AX_HOUDINI_INCLUDE_FILES}
  DESTINATION include/openvdb_ax_houdini/ax
  )
