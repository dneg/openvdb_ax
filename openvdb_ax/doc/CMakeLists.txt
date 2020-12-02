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

  CMake Configuration for OpenVDB AX Doxygen

#]=======================================================================]

cmake_minimum_required(VERSION 3.9)
project(OpenVDBAXDocs LANGUAGES NONE)

find_package(Doxygen REQUIRED dot)

set(DOXY_FILES
  ../
  ../ast
  ../cmd
  ../codegen
  ../compiler
  ../math
  ../python
  ../grammar
  ../grammar/generated
  ../test
  ax.txt
  axcplusplus.txt
  axexamples.txt
  axfunctionlist.txt
  doc.txt
  openvdbaxbuild.txt)

set(DOXYGEN_PROJECT_NAME "OpenVDB AX")
set(DOXYGEN_PROJECT_NUMBER "0.3.0")
set(DOXYGEN_PROJECT_BRIEF "")
set(DOXYGEN_FILE_PATTERNS "*.h") # headers only

set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_MAN NO)
set(DOXYGEN_GENERATE_LATEX NO)

set(DOXYGEN_EXTRA_PACKAGES amsmath) # for math formatting
set(DOXYGEN_FORMULA_FONTSIZE 14)

set(DOXYGEN_HTML_COLORSTYLE_HUE 4)
set(DOXYGEN_HTML_COLORSTYLE_SAT 222)
set(DOXYGEN_HTML_TIMESTAMP NO) # for simpler website diffs

set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_EXTRACT_STATIC YES)
set(DOXYGEN_EXTRACT_LOCAL_CLASSES NO)

set(DOXYGEN_ALPHABETICAL_INDEX NO)
set(DOXYGEN_DISTRIBUTE_GROUP_DOC YES)
set(DOXYGEN_FULL_PATH_NAMES NO)
set(DOXYGEN_GENERATE_TODOLIST NO)
set(DOXYGEN_HIDE_IN_BODY_DOCS YES)
set(DOXYGEN_HIDE_SCOPE_NAMES YES)
set(DOXYGEN_INLINE_INHERITED_MEMB YES)
set(DOXYGEN_MACRO_EXPANSION YES)

set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(DOXYGEN_QUIET YES)

set(DOXYGEN_ALIASES
  [[ijk="(<i>i</i>,&nbsp;<i>j</i>,&nbsp;<i>k</i>)"]]
  [[xyz="(<i>x</i>,&nbsp;<i>y</i>,&nbsp;<i>z</i>)"]]
  [[const="<tt>const</tt>"]]
  [[vdblink="@link @vdbnamespace"]]
  [[vdbnamespace="openvdb::v7_0"]])
set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)

set(DOXYGEN_PREDEFINED
  "OPENVDB_VERSION_NAME=v7_0"
  "__declspec(x):= __attribute__(x):=")

# Always add to build target (can't use USE_STAMP_FILE as we include
# complete directories in DOXY_FILES)
doxygen_add_docs(doc ${DOXY_FILES}
  ALL WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating API documentation with Doxygen")

# Suppress "Installing..." messages for all but one of the hundreds of generated files.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/html/index.html DESTINATION doc/html)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION doc MESSAGE_NEVER)
