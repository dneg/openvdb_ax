# Copyright (c) 2015-2018 DNEG Visual Effects
#
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
#
# Redistributions of source code must retain the above copyright
# and license notice and the following restrictions and disclaimer.
#
# *     Neither the name of DNEG Visual Effects nor the names
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
cmake_minimum_required(VERSION 3.1)
PROJECT ( OpenVDB_AX_Library )

set(CMAKE_CXX_STANDARD 11)

IF (NOT WIN32)
  FIND_PACKAGE ( Threads REQUIRED )
ENDIF()

# Determine OpenVDB version
FIND_PACKAGE ( OpenVDB REQUIRED )

# Set the variables that are otherwise defined in the core module.
SET ( OPENVDB_MAJOR_VERSION_NUMBER ${OpenVDB_MAJOR_VERSION} )
SET ( OPENVDB_MINOR_VERSION_NUMBER ${OpenVDB_MINOR_VERSION} )
SET ( OPENVDB_PATCH_VERSION_NUMBER ${OpenVDB_PATCH_VERSION} )

# Use the library installed
SET ( OPENVDB_SHARED_LIB ${OpenVDB_OPENVDB_LIBRARY} )

FIND_PACKAGE ( Boost ${MINIMUM_BOOST_VERSION} REQUIRED COMPONENTS random )
IF (WIN32 AND OPENVDB_AX_DISABLE_BOOST_IMPLICIT_LINKING)
  ADD_DEFINITIONS ( -DBOOST_ALL_NO_LIB )
ENDIF ()

FIND_PACKAGE ( Blosc REQUIRED )
FIND_PACKAGE ( TBB REQUIRED )
FIND_PACKAGE ( ILMBase REQUIRED )
FIND_PACKAGE ( LLVM REQUIRED )

IF (OPENVDB_AX_BUILD_DOCS)
  FIND_PACKAGE ( Doxygen REQUIRED )
ENDIF ()
IF (OPENVDB_AX_BUILD_UNITTESTS)
  FIND_PACKAGE ( CPPUnit REQUIRED )
ENDIF ()

IF ( NOT OPENVDB_AX_BUILD_HOUDINI_SOP )
  # If we are building OpenVDB AX Houdini SOP, we need to use the headers
  # shipped with Houdini to ensure version compatibility
  INCLUDE_DIRECTORIES ( SYSTEM ${Boost_INCLUDE_DIR} )
  INCLUDE_DIRECTORIES ( SYSTEM ${TBB_INCLUDE_DIR} )
ENDIF ()

INCLUDE_DIRECTORIES ( SYSTEM ${ILMBASE_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES ( SYSTEM ${LLVM_INCLUDE_DIR} )

INCLUDE_DIRECTORIES ( ${PROJECT_SOURCE_DIR} )

IF (WIN32)
  # Because of implicit linking!
  LINK_DIRECTORIES ( ${Boost_LIBRARY_DIR} )
ENDIF ()

INCLUDE_DIRECTORIES ( SYSTEM ${BLOSC_INCLUDE_DIR} )
INCLUDE_DIRECTORIES ( SYSTEM ${CPPUNIT_INCLUDE_DIR} )

IF (WIN32)
  ADD_DEFINITIONS ( -D_WIN32 -DNOMINMAX -DHALF_EXPORTS -DOPENVDB_DLL )
ELSE ()
  ADD_DEFINITIONS ( -pthread -fPIC )
ENDIF ()

SET ( OPENVDB_AX_LIBRARY_SOURCE_FILES
  ast/AST.cc
  ast/PrintTree.cc
  grammar/axlexer.cc
  grammar/axparser.cc
  codegen/ComputeGenerator.cc
  codegen/FunctionRegistry.cc
  codegen/FunctionTypes.cc
  codegen/Functions.cc
  codegen/PointComputeGenerator.cc
  codegen/PointFunctions.cc
  codegen/VolumeComputeGenerator.cc
  compiler/Compiler.cc
  compiler/PointExecutable.cc
  compiler/VolumeExecutable.cc
  )

SET_SOURCE_FILES_PROPERTIES ( ${OPENVDB_AX_LIBRARY_SOURCE_FILES}
  PROPERTIES
  COMPILE_FLAGS "-DOPENVDB_PRIVATE -DOPENVDB_USE_BLOSC"
  )

ADD_LIBRARY ( openvdb_ax_static STATIC
  ${OPENVDB_AX_LIBRARY_SOURCE_FILES}
  )
ADD_LIBRARY ( openvdb_ax_shared SHARED
  ${OPENVDB_AX_LIBRARY_SOURCE_FILES}
  )

TARGET_LINK_LIBRARIES ( openvdb_ax_static
  ${OPENVDB_SHARED_LIB}
  ${Boost_RANDOM_LIBRARY}
  ${Tbb_TBB_LIBRARY}
  ${Ilmbase_HALF_LIBRARY}
  ${BLOSC_blosc_LIBRARY}
  ${LLVM_LIBRARIES}
  )
TARGET_LINK_LIBRARIES ( openvdb_ax_shared
  ${OPENVDB_SHARED_LIB}
  ${Boost_RANDOM_LIBRARY}
  ${Tbb_TBB_LIBRARY}
  ${Ilmbase_HALF_LIBRARY}
  ${BLOSC_blosc_LIBRARY}
  ${LLVM_LIBRARIES}
  )

IF (WIN32)
  SET_TARGET_PROPERTIES ( openvdb_ax_static
    PROPERTIES OUTPUT_NAME libopenvdb_ax
    )
ELSE()
  SET_TARGET_PROPERTIES ( openvdb_ax_static
    PROPERTIES OUTPUT_NAME openvdb_ax
    )
ENDIF()

SET_TARGET_PROPERTIES(
  openvdb_ax_shared
  PROPERTIES
  OUTPUT_NAME openvdb_ax
  )

SET ( VDB_AX_SOURCE_FILES  cmd/openvdb_ax/main.cc )
SET_SOURCE_FILES_PROPERTIES ( ${VDB_AX_SOURCE_FILES}
  PROPERTIES
  COMPILE_FLAGS "-DOPENVDB_USE_BLOSC"
  )

ADD_EXECUTABLE ( vdb_ax
  ${VDB_AX_SOURCE_FILES}
  )

TARGET_LINK_LIBRARIES ( vdb_ax
  openvdb_ax_shared
  ${OPENVDB_SHARED_LIB}
  ${CMAKE_THREAD_LIBS_INIT}
  ${BLOSC_blosc_LIBRARY}
  ${LLVM_LIBRARIES}
  stdc++
  )


SET ( TEST_SOURCE_FILES
  test/backend/TestFunctionBase.cc
  test/backend/TestFunctionSignature.cc
  test/backend/TestSymbolTable.cc
  test/frontend/TestAttributeAssignExpressionNode.cc
  test/frontend/TestAttributeValueNode.cc
  test/frontend/TestBinaryOperatorNode.cc
  test/frontend/TestCastNode.cc
  test/frontend/TestConditionalStatementNode.cc
  test/frontend/TestCrementNode.cc
  test/frontend/TestDeclareLocalNode.cc
  test/frontend/TestFunctionCallNode.cc
  test/frontend/TestKeywordNode.cc
  test/frontend/TestLocalValueNode.cc
  test/frontend/TestSyntaxFailures.cc
  test/frontend/TestUnaryOperatorNode.cc
  test/frontend/TestValueNode.cc
  test/frontend/TestVectorPack.cc
  test/frontend/TestVectorUnpack.cc
  test/integration/CompareGrids.cc
  test/integration/TestAssign.cc
  test/integration/TestBinary.cc
  test/integration/TestCast.cc
  test/integration/TestChannelExpressions.cc
  test/integration/TestDeclare.cc
  test/integration/TestEditGroups.cc
  test/integration/TestEmpty.cc
  test/integration/TestFunction.cc
  test/integration/TestGroups.cc
  test/integration/TestHarness.cc
  test/integration/TestKeyword.cc
  # test/integration/TestString.cc @todo: reenable string tests with string support
  test/integration/TestUnary.cc
  test/integration/TestWorldSpaceAccessors.cc
  test/main.cc
  )

SET_SOURCE_FILES_PROPERTIES ( ${TEST_SOURCE_FILES}
  PROPERTIES
  COMPILE_FLAGS "-DOPENVDB_USE_BLOSC"
  )

IF ( OPENVDB_AX_BUILD_UNITTESTS )

  # this copies the AX snippets into the build directory so that the test executable
  # can find them correctly
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test/snippets ${CMAKE_CURRENT_BINARY_DIR}/test/snippets)

  ADD_EXECUTABLE ( vdb_ax_test
    ${TEST_SOURCE_FILES}
    )

  TARGET_LINK_LIBRARIES ( vdb_ax_test
    ${CPPUnit_cppunit_LIBRARY}
    openvdb_ax_shared
    stdc++
    )

  ADD_TEST ( vdb_ax_unit_test vdb_ax_test )

ENDIF (OPENVDB_AX_BUILD_UNITTESTS)

# Doxygen docmentation
IF (OPENVDB_AX_BUILD_DOCS)

  FILE ( READ ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-config DOXYGEN_CONFIG_CONTENT )
  FILE ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake-doxygen-config ${DOXYGEN_CONFIG_CONTENT} )
  FILE ( APPEND ${CMAKE_CURRENT_BINARY_DIR}/cmake-doxygen-config "OUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/doc\n" )

  ADD_CUSTOM_TARGET ( doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/cmake-doxygen-config
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )

  INSTALL ( CODE "EXECUTE_PROCESS(COMMAND \"${CMAKE_COMMAND}\" --build  \"${PROJECT_BINARY_DIR}\" --target doc)")
  INSTALL ( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html DESTINATION share/docs/openvdb_ax)

ENDIF ()

# Installation
INSTALL ( TARGETS
  vdb_ax
  DESTINATION
  bin
  )

INSTALL ( TARGETS
  openvdb_ax_static
  DESTINATION
  lib
  )

IF (WIN32)
  INSTALL ( TARGETS
    openvdb_ax_shared
    RUNTIME DESTINATION bin COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Runtime
    )
ELSE()
  INSTALL ( TARGETS
    openvdb_ax_shared
    DESTINATION
    lib
    )
ENDIF ()

# install headers
# todo: can this be done without splitting these into multiple lists manually?

SET ( OPENVDB_AX_AST_INCLUDE_FILES
  ast/AST.h
  ast/Literals.h
  ast/PrintTree.h
  ast/Scanners.h
  ast/Tokens.h
)

SET ( OPENVDB_AX_CODEGEN_INCLUDE_FILES
  codegen/ComputeGenerator.h
  codegen/FunctionRegistry.h
  codegen/FunctionTypes.h
  codegen/Functions.h
  codegen/LeafLocalData.h
  codegen/PointComputeGenerator.h
  codegen/PointFunctions.h
  codegen/SymbolTable.h
  codegen/Types.h
  codegen/Utils.h
  codegen/VolumeComputeGenerator.h
  codegen/VolumeFunctions.h
)

SET ( OPENVDB_AX_COMPILER_INCLUDE_FILES
  compiler/Compiler.h
  compiler/CompilerOptions.h
  compiler/CustomData.h
  compiler/TargetRegistry.h
  compiler/PointExecutable.h
  compiler/VolumeExecutable.h
)

INSTALL (FILES Exceptions.h DESTINATION include/openvdb_ax/)
INSTALL (FILES ${OPENVDB_AX_AST_INCLUDE_FILES} DESTINATION include/openvdb_ax/ast)
INSTALL (FILES ${OPENVDB_AX_CODEGEN_INCLUDE_FILES} DESTINATION include/openvdb_ax/codegen)
INSTALL (FILES ${OPENVDB_AX_COMPILER_INCLUDE_FILES} DESTINATION include/openvdb_ax/compiler)
