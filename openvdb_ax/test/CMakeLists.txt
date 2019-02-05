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

FIND_PACKAGE ( CPPUnit REQUIRED )

SET ( TEST_SOURCE_FILES
  backend/TestFunctionBase.cc
  backend/TestFunctionSignature.cc
  backend/TestSymbolTable.cc
  compiler/TestPointExecutable.cc
  compiler/TestVolumeExecutable.cc
  frontend/TestAttributeAssignExpressionNode.cc
  frontend/TestAttributeValueNode.cc
  frontend/TestBinaryOperatorNode.cc
  frontend/TestCastNode.cc
  frontend/TestConditionalStatementNode.cc
  frontend/TestCrementNode.cc
  frontend/TestDeclareLocalNode.cc
  frontend/TestExternalVariableNode.cc
  frontend/TestFunctionCallNode.cc
  frontend/TestKeywordNode.cc
  frontend/TestLocalValueNode.cc
  frontend/TestSyntaxFailures.cc
  frontend/TestUnaryOperatorNode.cc
  frontend/TestValueNode.cc
  frontend/TestVectorPack.cc
  frontend/TestVectorUnpack.cc
  integration/CompareGrids.cc
  integration/TestAssign.cc
  integration/TestBinary.cc
  integration/TestCast.cc
  integration/TestDeclare.cc
  integration/TestEditGroups.cc
  integration/TestEmpty.cc
  integration/TestFunction.cc
  integration/TestGroups.cc
  integration/TestHarness.cc
  integration/TestKeyword.cc
  # integration/TestString.cc @todo: reenable string tests with string support
  integration/TestUnary.cc
  integration/TestWorldSpaceAccessors.cc
  main.cc
  )

# this copies the AX snippets into the build directory so that the test executable
# can find them correctly
EXECUTE_PROCESS ( COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/snippets/ ${CMAKE_CURRENT_BINARY_DIR}/test/snippets )

ADD_EXECUTABLE ( vdb_ax_test
    ${TEST_SOURCE_FILES}
)

TARGET_LINK_LIBRARIES ( vdb_ax_test
    ${CPPUnit_cppunit_LIBRARY}
    openvdb_ax_shared
    stdc++
)

ADD_TEST ( vdb_ax_unit_test vdb_ax_test )
