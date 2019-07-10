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
#[=======================================================================[

  CMake Configuration for OpenVDB Unit Tests

#]=======================================================================]

project(OpenVDBAXUnitTests)
cmake_minimum_required(VERSION 3.3)
# Monitoring <PackageName>_ROOT variables
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

##########################################################################

message(STATUS "----------------------------------------------------")
message(STATUS "---------- Configuring OpenVDBAXUnitTests ----------")
message(STATUS "----------------------------------------------------")

##########################################################################

if(NOT OPENVDB_BUILD_AX)
  set(OPENVDBAX_LIB OpenVDB::openvdb_ax)
else()
  set(OPENVDBAX_LIB openvdb_ax)
endif()

find_package(CppUnit REQUIRED)

set(TEST_SOURCE_FILES
  ast/TestScanners.cc
  backend/TestFunctionBase.cc
  backend/TestFunctionSignature.cc
  backend/TestSymbolTable.cc
  compiler/TestPointExecutable.cc
  compiler/TestVolumeExecutable.cc
  frontend/TestArrayPack.cc
  frontend/TestArrayUnpack.cc
  frontend/TestAssignExpressionNode.cc
  frontend/TestAttributeNode.cc
  frontend/TestBinaryOperatorNode.cc
  frontend/TestCastNode.cc
  frontend/TestConditionalStatementNode.cc
  frontend/TestCrementNode.cc
  frontend/TestDeclareLocalNode.cc
  frontend/TestExternalVariableNode.cc
  frontend/TestFunctionCallNode.cc
  frontend/TestKeywordNode.cc
  frontend/TestLocalNode.cc
  frontend/TestLoopNode.cc
  frontend/TestSyntaxFailures.cc
  frontend/TestUnaryOperatorNode.cc
  frontend/TestValueNode.cc
  integration/CompareGrids.cc
  integration/TestAssign.cc
  integration/TestBinary.cc
  integration/TestCast.cc
  integration/TestConditional.cc
  integration/TestDeclare.cc
  integration/TestEditGroups.cc
  integration/TestEmpty.cc
  integration/TestFunction.cc
  integration/TestGroups.cc
  integration/TestHarness.cc
  integration/TestKeyword.cc
  integration/TestLoop.cc
  integration/TestString.cc
  integration/TestUnary.cc
  integration/TestWorldSpaceAccessors.cc
  main.cc
  )

add_executable(vdb_ax_test
  ${TEST_SOURCE_FILES}
)

target_link_libraries(vdb_ax_test
  ${OPENVDBAX_LIB}
  CppUnit::cppunit
)
target_include_directories(vdb_ax_test
  PRIVATE ../ .
)

add_test(NAME vdb_ax_unit_test COMMAND vdb_ax_test -v WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../)