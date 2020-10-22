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

  CMake Configuration for OpenVDB AX

#]=======================================================================]

cmake_minimum_required(VERSION 3.12)
project(OpenVDBAX LANGUAGES CXX)

include(CMakeDependentOption)
include(GNUInstallDirs)

option(OPENVDB_BUILD_AX "Build the OpenVDB AX library" ON)
option(OPENVDB_BUILD_AX_BINARIES "Build the OpenVDB AX command line binary" ON)
option(OPENVDB_BUILD_AX_PYTHON_MODULE "Build the OpenVDB AX Python module" OFF)
option(OPENVDB_BUILD_AX_HOUDINI_PLUGIN "Build the OpenVDB AX Houdini plugin" OFF)
option(OPENVDB_BUILD_AX_UNITTESTS "Build the OpenVDB AX unit tests" OFF)
option(OPENVDB_BUILD_AX_DOCS "Build the OpenVDB AX documentation" OFF)
option(USE_CCACHE "Build using Ccache if found on the path" ON)
option(USE_HOUDINI "Build the library against a Houdini installation." OFF)
option(DISABLE_CMAKE_SEARCH_PATHS [=[
Disable CMakes default system search paths when locating dependencies. When enabled, CMake will fall back to
its default system search routine if it cannot find a dependency with the provided settings. When disabled, only
paths provided through the Xxx_ROOT, supported XXX_INCLUDEDIR/XXX_LIBRARYDIR variables or the SYSTEM_LIBRARY_PATHS
list will be searched.]=] OFF)

set(_OPENVDB_SIMD_OPTIONS None SSE42 AVX)
if(NOT OPENVDB_SIMD)
  set(OPENVDB_SIMD None CACHE STRING
    "Choose whether to enable SIMD compiler flags or not, options are: None SSE42 AVX.
    Although not required, it is strongly recommended to enable SIMD. AVX implies SSE42.
    None is the default." FORCE
  )
elseif(NOT ${OPENVDB_SIMD} IN_LIST _OPENVDB_SIMD_OPTIONS)
  message(WARNING "Unrecognized or unsupported value for OPENVDB_SIMD, "
    "using None instead.")
  set(OPENVDB_SIMD None CACHE STRING FORCE)
endif()

###### Deprecated options

if(DEFINED USE_SYSTEM_LIBRARY_PATHS)
  message(DEPRECATION "The USE_SYSTEM_LIBRARY_PATHS option is deprecated and will be removed. "
    "Use DISABLE_CMAKE_SEARCH_PATHS.")
  if(USE_SYSTEM_LIBRARY_PATHS)
    set(DISABLE_CMAKE_SEARCH_PATHS OFF)
  else()
    set(DISABLE_CMAKE_SEARCH_PATHS ON)
  endif()
endif()

##### Dependent options
cmake_dependent_option(OPENVDB_BUILD_AX_GRAMMAR "Rebuild the OpenVDB AX grammar. CMake 3.4 with flex and bison."
  ON "OPENVDB_BUILD_AX" OFF)

#########################################################################

# General CMake and CXX settings

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14 CACHE STRING
    "The C++ standard whose features are requested to build this target." FORCE)
elseif(CMAKE_CXX_STANDARD LESS 14)
  message(FATAL_ERROR "Provided C++ Standard is less than the supported minimum."
    "Required is at least \"14\" (found ${CMAKE_CXX_STANDARD})")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

if(OPENVDB_ENABLE_RPATH)
  # Configure rpath for installation base on the following:
  # https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
  set(CMAKE_SKIP_BUILD_RPATH FALSE)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# For CMake's find Threads module which brings in pthread - This flag
# forces the compiler -pthread flag vs -lpthread
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

enable_testing()

# Add our cmake modules and the VDB backports in case we're building against
# VDB pre 6.1.0

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/cmake;${CMAKE_CURRENT_LIST_DIR}/cmake/backports")

if(OPENVDB_BUILD_AX_HOUDINI_PLUGIN)
  set(USE_HOUDINI ON)
endif()

# Can't include OpenVDBHoudiniSetup twice and finding openvdb_houdini
# will cause it to be called, so only include here if not building the
# openvdb_houdini lib

if(USE_HOUDINI AND NOT OPENVDB_BUILD_AX_HOUDINI_PLUGIN)
  include(OpenVDBHoudiniSetup)
endif()

#########################################################################

# Add the doxygen command if required - do this here so we guarantee not
# to error on unrelated build issues

if(OPENVDB_BUILD_AX_DOCS)
  add_subdirectory(openvdb_ax/doc)
endif()

#########################################################################
# ccache setup

if(USE_CCACHE)
  find_program(CCACHE_PATH ccache)
  if(CCACHE_PATH)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    message(STATUS "Using ccache: ${CCACHE_PATH}")
  endif()
endif()

# Build type configuration - default to Release if none is set

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE
  )
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")

#########################################################################

# Compiler configuration. Add definitions for a number of compiler warnings
# for sub projects and verify version requirements

set(HAS_AVAILABLE_WARNINGS FALSE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(OPENVDB_CXX_STRICT)
    message(STATUS "Configuring Clang CXX warnings")
    set(HAS_AVAILABLE_WARNINGS TRUE)
    add_compile_options(
      -Werror
      -Wall
      -Wextra
      -Wconversion
      -Wno-sign-conversion
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(OPENVDB_CXX_STRICT)
    message(STATUS "Configuring GCC CXX warnings")
    set(HAS_AVAILABLE_WARNINGS TRUE)
    add_compile_options(
      -Werror
      -Wall
      -Wextra
      -pedantic
      -Wcast-align
      -Wcast-qual
      -Wconversion
      -Wdisabled-optimization
      -Woverloaded-virtual
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Increase the number of sections that an object file can contain
  add_compile_options(/bigobj)
  # Math constants are not included in <cmath> unless _USE_MATH_DEFINES is
  # defined on MSVC
  # https://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
  add_definitions(-D_USE_MATH_DEFINES)
  # Disable the non-portable Windows definitions of min() and max() macros
  add_definitions(-DNOMINMAX)
  # Excludes APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  # Disable non-secure CRT library function warnings
  # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/
  #     compiler-warning-level-3-c4996?view=vs-2019#unsafe-crt-library-functions
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # Disable POSIX function name warnings
  # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/
  #     compiler-warning-level-3-c4996?view=vs-2019#posix-function-names
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
  if(NOT OPENVDB_CXX_STRICT)
    message(STATUS "Suppressing some noisy MSVC CXX warnings, "
      "set OPENVDB_CXX_STRICT=ON to re-enable them.")
    # Conversion from int64_t to long
    add_compile_options(/wd4244)
    # It's not possible to use STL types in DLL interfaces in a portable and
    # reliable way so disable this warning
    add_compile_options(/wd4251)
    # Conversion from size_t to uLong
    add_compile_options(/wd4267)
    # Non dll-interface class used as base for dll-interface class
    add_compile_options(/wd4275)
    # Truncation from 'int' to 'bool'
    add_compile_options(/wd4305)
  endif()
endif()

if(OPENVDB_CXX_STRICT AND NOT HAS_AVAILABLE_WARNINGS)
  message(WARNING "No available CXX warnings for compiler ${CMAKE_CXX_COMPILER_ID}")
endif()
unset(HAS_AVAILABLE_WARNINGS)

# Configure SIMD. AVX implies SSE 4.2.

if(OPENVDB_SIMD STREQUAL "AVX")
  add_compile_options(-mavx -msse4.2)
  add_definitions(-DOPENVDB_USE_AVX)
  add_definitions(-DOPENVDB_USE_SSE42)
elseif(OPENVDB_SIMD STREQUAL "SSE42")
  add_compile_options(-msse4.2)
  add_definitions(-DOPENVDB_USE_SSE42)
endif()

##########################################################################

set(REQUIRED_OPENVDB_COMPONENTS openvdb)
if (OPENVDB_BUILD_AX_HOUDINI_PLUGIN)
  list(APPEND REQUIRED_OPENVDB_COMPONENTS openvdb_houdini)
endif()
if (OPENVDB_BUILD_AX_PYTHON_MODULE)
  list(APPEND REQUIRED_OPENVDB_COMPONENTS pyopenvdb)
endif()
if (NOT OPENVDB_BUILD_AX)
  list(APPEND REQUIRED_OPENVDB_COMPONENTS openvdb_ax)
endif()

find_package(OpenVDB REQUIRED COMPONENTS ${REQUIRED_OPENVDB_COMPONENTS})

##########################################################################

# Detect matrix grid support from the core library, if available

execute_process(COMMAND ${CMAKE_COMMAND} -E echo_append "-- Detecting support for OpenVDB Matrix Grids...")

if(NOT DEFINED MATRIX_SUPPORT)
  # Try and build a file which instantiates a mat3f grid
  set(MAT_GRID_CODE_TEST "#include <openvdb/openvdb.h>
    using T = openvdb::BoolGrid::ValueConverter<openvdb::math::Mat3<float>>::Type\;
    int main() { T grid\; return 0\; }")
  set(MAT_GRID_TEST ${CMAKE_CURRENT_BINARY_DIR}/tmp.cc)
  file(WRITE ${MAT_GRID_TEST} ${MAT_GRID_CODE_TEST})
  try_compile(MATRIX_SUPPORT ${CMAKE_CURRENT_BINARY_DIR} ${MAT_GRID_TEST}
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${OpenVDB_INCLUDE_DIRS};${IlmBase_INCLUDE_DIRS};${Tbb_INCLUDE_DIRS}"
    LINK_LIBRARIES ${OpenVDB_openvdb_LIBRARY} ${IlmBase_LIBRARIES} ${Tbb_LIBRARIES}
    COMPILE_DEFINITIONS ${OpenVDB_DEFINITIONS}
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo " ${MATRIX_SUPPORT}")

#########################################################################

if(OPENVDB_BUILD_AX)
  add_subdirectory(openvdb_ax)
endif()

if(OPENVDB_BUILD_AX_BINARIES)
  add_subdirectory(openvdb_ax/cmd)
endif()

if(OPENVDB_BUILD_AX_UNITTESTS)
  add_subdirectory(openvdb_ax/test)
endif()

if(OPENVDB_BUILD_AX_PYTHON_MODULE)
  add_subdirectory(openvdb_ax/python)
endif()

if(OPENVDB_BUILD_AX_HOUDINI_PLUGIN)
  add_subdirectory(openvdb_ax_houdini)
endif()
