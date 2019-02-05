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

# -*- cmake -*-
# - Find OpenVDB Houdini
#
# Author : Richard Jones
#
# OPENVDB_HOUDINI_FOUND               Set if OpenVDB Houdini is found.
# OPENVDB_HOUDINI_INCLUDE_DIR         OpenVDB Houdini's include directory
# OPENVDB_HOUDINI_UTILS_INCLUDE_DIR   OpenVDB Houdini's Utils include directory
# OPENVDB_HOUDINI_LIBRARY_DIR         OpenVDB Houdini's library directory
# OPENVDB_HOUDINI_LIBRARY             OpenVDB Houdini's library
#

find_package( PackageHandleStandardArgs )

find_path( OPENVDB_HOUDINI_LOCATION include/openvdb_houdini/
  "$ENV{OPENVDB_HOUDINI_ROOT}"
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  )

find_path( OPENVDB_HOUDINI_UTILS_LOCATION include/houdini_utils/
  "$ENV{OPENVDB_HOUDINI_ROOT}"
  NO_DEFAULT_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  )

find_package_handle_standard_args( OpenVDB_Houdini
  REQUIRED_VARS OPENVDB_HOUDINI_LOCATION
  )

if( OPENVDB_HOUDINI_FOUND )
  set( OPENVDB_HOUDINI_INCLUDE_DIR ${OPENVDB_HOUDINI_LOCATION}/include
    CACHE PATH "OpenVDB include directory")

  set( OPENVDB_HOUDINI_UTILS_INCLUDE_DIR ${OPENVDB_HOUDINI_UTILS_LOCATION}/include
    CACHE PATH "OpenVDB Houdini Utils include directory")

  set( OPENVDB_HOUDINI_LIBRARY_DIR ${OPENVDB_HOUDINI_LOCATION}/lib
    CACHE PATH "OpenVDB Houdini library directory" )

  find_library( OPENVDB_HOUDINI_LIBRARY openvdb_houdini
    PATHS ${OPENVDB_HOUDINI_LIBRARY_DIR}
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    )

endif( OPENVDB_HOUDINI_FOUND )
