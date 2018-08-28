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
#
# Makefile for the OpenVDB AX Compiler library

# See INSTALL for a list of requirements.
#
# Targets:
#   lib                 the OpenVDB AX library
#
#   doc                 HTML documentation (doc/html/index.html)
#   pdfdoc              PDF documentation (doc/latex/refman.pdf;
#                       requires LaTeX and ghostscript)
#   vdb_ax              command-line tool to compile and run ax
#   vdb_test            unit tests for the OpenVDB library
#
#   all                 [default target] all of the above
#   install_lib         install the lib and the headers
#   install             install all of the above except vdb_test
#                       into subdirectories of DESTDIR
#   depend              recompute source file header dependencies
#   header_test         check for missing or indirectly included headers
#   clean               delete generated files from the local directory
#   test                run tests
#
# Options:
#   abi=N               build for compatibility with version N of the
#                       OpenVDB Grid ABI, where N is 2, 3, 4, etc.
#                       (some newer features will be disabled)
#   shared=no           link executables against static OpenVDB libraries
#                       (default: link against shared libraries)
#   debug=yes           build with debugging symbols and without optimization
#   verbose=yes         run commands (e.g., doxygen) in verbose mode


#
# The following variables must be defined, either here or on the command line
# (e.g., "make install DESTDIR=/usr/local"):
#
# Note that if you plan to build the Houdini OpenVDB AX tool (distributed
# separately), you must build the OpenVDB AX library and the OpenVDB Houdini tools
# against compatible versions of the Boost, OpenEXR and TBB libraries.
# Fortunately, all three are included in the Houdini HDK, so the relevant
# variables below point by default to the HDK library and header directories:
# $(HDSO) and $(HT)/include, respectively.  (Source the houdini_setup script
# to set those two environment variables.)
#

# The directory into which to install libraries, executables and header files
DESTDIR := /tmp/OpenVDB

# The directory into which to install libraries (e.g., for Linux multiarch support)
DESTDIR_LIB_DIR := $(DESTDIR)/lib

# The parent directory of the openvdb/ header directory
OPENVDB_INCL_DIR := $(DESTDIR)/include
# The directory containing libopenvdb
OPENVDB_LIB_DIR := $(DESTDIR)/lib
OPENVDB_LIB := -lopenvdb

# The parent directory of the boost/ header directory
BOOST_INCL_DIR := $(HT)/include
# The directory containing libboost_random
BOOST_LIB_DIR := $(HDSO)
BOOST_LIB := -lboost_random

# The parent directory of the OpenEXR/ header directory (which contains half.h)
ILMBASE_INCL_DIR := $(HT)/include
# The directory containing libHalf
ILMBASE_LIB_DIR := $(HDSO)
HALF_LIB := -lHalf

# The parent directory of the tbb/ header directory
TBB_INCL_DIR := $(HT)/include
# The directory containing libtbb
TBB_LIB_DIR := $(HDSO)
TBB_LIB := -ltbb

# The parent directory of the blosc.h header
# (leave blank if Blosc is unavailable)
BLOSC_INCL_DIR := $(HT)/include
# The directory containing libblosc
BLOSC_LIB_DIR := $(HDSO)
BLOSC_LIB := -lblosc

# The LLVM directory containing bin, lib, include, etc.
LLVM_ROOT := /apps/Linux64/llvm/llvm-5.0
LLVM_INCL_DIR := $(LLVM_ROOT)/include
LLVM_LIB_DIR := $(LLVM_ROOT)/lib
LLVM_LIB := $(shell $(LLVM_ROOT)/bin/llvm-config --libs all)

FLEX_BIN := flex
BISON_BIN := bison

# A scalable, concurrent malloc replacement library
# such as jemalloc (included in the Houdini HDK) or TBB malloc
# (leave blank if unavailable)
CONCURRENT_MALLOC_LIB := -ljemalloc
#CONCURRENT_MALLOC_LIB := -ltbbmalloc_proxy -ltbbmalloc
# The directory containing the malloc replacement library
CONCURRENT_MALLOC_LIB_DIR := $(HDSO)

# The parent directory of the cppunit/ header directory
# (leave blank if CppUnit is unavailable)
CPPUNIT_INCL_DIR := /rel/map/generic-2013.22/sys_include
# The directory containing libcppunit
CPPUNIT_LIB_DIR := /rel/depot/third_party_build/cppunit/1.10.2-7/opt-ws5-x86_64-gccWS5_64/lib
CPPUNIT_LIB := -lcppunit

# The parent directory of the log4cplus/ header directory
# (leave blank if log4cplus is unavailable)
LOG4CPLUS_INCL_DIR := /rel/folio/log4cplus/log4cplus-1.1.2-latest/sys_include
# The directory containing liblog4cplus
LOG4CPLUS_LIB_DIR := /rel/folio/log4cplus/log4cplus-1.1.2-latest/library/c++11
LOG4CPLUS_LIB := -llog4cplus

# The Doxygen executable
# (leave blank if Doxygen is unavailable)
DOXYGEN := doxygen


#
# Ideally, users shouldn't need to change anything below this line.
#

SHELL = /bin/bash

# Turn off implicit rules for speed.
.SUFFIXES:

ifneq (,$(INSTALL_DIR))
    $(warning Warning: $$(INSTALL_DIR) is no longer used; set $$(DESTDIR) instead.)
endif

# Determine the platform.
ifeq ("$(OS)","Windows_NT")
    WINDOWS_NT := 1
else
    UNAME_S := $(shell uname -s)
    ifeq ("$(UNAME_S)","Linux")
        LINUX := 1
    else
        ifeq ("$(UNAME_S)","Darwin")
            MBSD := 1
        endif
    endif
endif

ifeq (yes,$(strip $(debug)))
    OPTIMIZE := -g
else
    OPTIMIZE := -O3 -DNDEBUG
endif

ifeq (no,$(strip $(verbose)))
    QUIET := > /dev/null
    QUIET_TEST := $(QUIET)
else
    QUIET :=
    QUIET_TEST := -v
endif

has_blosc := no
ifneq (,$(and $(BLOSC_LIB_DIR),$(BLOSC_INCL_DIR),$(BLOSC_LIB)))
    has_blosc := yes
endif

has_cppunit := no
ifneq (,$(and $(CPPUNIT_INCL_DIR),$(CPPUNIT_LIB_DIR),$(CPPUNIT_LIB)))
    has_cppunit := yes
endif

has_log4cplus := no
ifneq (,$(and $(LOG4CPLUS_LIB_DIR),$(LOG4CPLUS_INCL_DIR),$(LOG4CPLUS_LIB)))
    has_log4cplus := yes
endif

INCLDIRS ?= -I . -I .. -I $(OPENVDB_INCL_DIR) -isystem $(BOOST_INCL_DIR) -isystem $(ILMBASE_INCL_DIR) -isystem $(TBB_INCL_DIR) -isystem $(LLVM_INCL_DIR)
ifeq (yes,$(has_blosc))
    INCLDIRS += -isystem $(BLOSC_INCL_DIR)
endif
ifeq (yes,$(has_log4cplus))
    INCLDIRS += -isystem $(LOG4CPLUS_INCL_DIR)
endif

CXXFLAGS += -std=c++11
CXXFLAGS += -pthread $(OPTIMIZE) $(INCLDIRS)
CXXFLAGS += -Wall

ifeq (yes,$(has_blosc))
    CXXFLAGS += -DOPENVDB_USE_BLOSC
endif
ifeq (yes,$(has_log4cplus))
    CXXFLAGS += -DOPENVDB_USE_LOG4CPLUS
endif
abi := $(strip $(abi))
ifneq (,$(abi))
    CXXFLAGS += -DOPENVDB_3_ABI_COMPATIBLE  # TODO: deprecated
    CXXFLAGS += -DOPENVDB_ABI_VERSION_NUMBER=$(abi)
endif

LIBS := \
    -ldl -lm -lz \
    -L$(OPENVDB_LIB_DIR) $(OPENVDB_LIB) \
    -L$(ILMBASE_LIB_DIR) $(HALF_LIB) \
    -L$(TBB_LIB_DIR) $(TBB_LIB) \
    -L$(BOOST_LIB_DIR) $(BOOST_LIB) \
    -L$(LLVM_LIB_DIR) $(LLVM_LIB) \
#
LIBS_RPATH := \
    -ldl -lm -lz \
    -Wl,-rpath,$(OPENVDB_LIB_DIR) -L$(OPENVDB_LIB_DIR) $(OPENVDB_LIB) \
    -Wl,-rpath,$(ILMBASE_LIB_DIR) -L$(ILMBASE_LIB_DIR) $(HALF_LIB) \
    -Wl,-rpath,$(TBB_LIB_DIR) -L$(TBB_LIB_DIR) $(TBB_LIB) \
    -Wl,-rpath,$(BOOST_LIB_DIR) -L$(BOOST_LIB_DIR) $(BOOST_LIB) \
    -Wl,-rpath,$(LLVM_LIB_DIR) -L$(LLVM_LIB_DIR) $(LLVM_LIB) \
#
ifeq (yes,$(has_blosc))
    LIBS += -L$(BLOSC_LIB_DIR) $(BLOSC_LIB)
    LIBS_RPATH += -Wl,-rpath,$(BLOSC_LIB_DIR) -L$(BLOSC_LIB_DIR) $(BLOSC_LIB)
endif
ifeq (yes,$(has_log4cplus))
    LIBS += -L$(LOG4CPLUS_LIB_DIR) $(LOG4CPLUS_LIB)
    LIBS_RPATH += -Wl,-rpath,$(LOG4CPLUS_LIB_DIR) -L$(LOG4CPLUS_LIB_DIR) $(LOG4CPLUS_LIB)
endif
ifneq (,$(strip $(CONCURRENT_MALLOC_LIB)))
ifneq (,$(strip $(CONCURRENT_MALLOC_LIB_DIR)))
    LIBS_RPATH += -Wl,-rpath,$(CONCURRENT_MALLOC_LIB_DIR) -L$(CONCURRENT_MALLOC_LIB_DIR)
endif
endif
ifdef LINUX
    LIBS += -lrt
    LIBS_RPATH += -lrt
endif

INCLUDE_NAMES := Exceptions.h \
                 ast/AST.h \
                 ast/Literals.h \
                 ast/PrintTree.h \
                 ast/Scanners.h \
                 ast/Tokens.h \
                 codegen/ComputeGenerator.h \
                 codegen/FunctionRegistry.h \
                 codegen/FunctionTypes.h \
                 codegen/Functions.h \
                 codegen/LeafLocalData.h \
                 codegen/PointComputeGenerator.h \
                 codegen/PointFunctions.h \
                 codegen/SymbolTable.h \
                 codegen/Types.h \
                 codegen/Utils.h \
                 codegen/VolumeComputeGenerator.h \
                 codegen/VolumeFunctions.h \
                 compiler/Compiler.h \
                 compiler/CompilerOptions.h \
                 compiler/CustomData.h \
                 compiler/TargetRegistry.h \
                 compiler/PointExecutable.h \
                 compiler/VolumeExecutable.h \
#

SRC_NAMES := ast/AST.cc \
             ast/PrintTree.cc \
             grammar/axlexer.cc \
             grammar/axparser.cc \
             codegen/ComputeGenerator.cc \
             codegen/FunctionRegistry.cc \
             codegen/FunctionTypes.cc \
             codegen/Functions.cc \
             codegen/PointComputeGenerator.cc \
             codegen/PointFunctions.cc \
             codegen/VolumeComputeGenerator.cc \
             compiler/Compiler.cc \
             compiler/PointExecutable.cc \
             compiler/VolumeExecutable.cc \
#

TEST_INCLUDE_NAMES := \
    test/backend/util.h \
    test/integration/CompareGrids.h \
    test/integration/TestHarness.h \
    test/util.h \
#

TEST_SRC_NAMES := \
    test/backend/TestFunctionBase.cc \
    test/backend/TestFunctionSignature.cc \
    test/backend/TestSymbolTable.cc \
    test/frontend/TestAttributeAssignExpressionNode.cc \
    test/frontend/TestAttributeValueNode.cc \
    test/frontend/TestBinaryOperatorNode.cc \
    test/frontend/TestCastNode.cc \
    test/frontend/TestConditionalStatementNode.cc \
    test/frontend/TestCrementNode.cc \
    test/frontend/TestDeclareLocalNode.cc \
    test/frontend/TestFunctionCallNode.cc \
    test/frontend/TestKeywordNode.cc \
    test/frontend/TestLocalValueNode.cc \
    test/frontend/TestSyntaxFailures.cc \
    test/frontend/TestUnaryOperatorNode.cc \
    test/frontend/TestValueNode.cc \
    test/frontend/TestVectorPack.cc \
    test/frontend/TestVectorUnpack.cc \
    test/main.cc \
    test/integration/CompareGrids.cc \
    test/integration/TestAssign.cc \
    test/integration/TestBinary.cc \
    test/integration/TestCast.cc \
    test/integration/TestChannelExpressions.cc \
    test/integration/TestDeclare.cc \
    test/integration/TestEditGroups.cc \
    test/integration/TestEmpty.cc \
    test/integration/TestFunction.cc \
    test/integration/TestGroups.cc \
    test/integration/TestHarness.cc \
    test/integration/TestKeyword.cc \
    test/integration/TestUnary.cc \
    test/integration/TestWorldSpaceAccessors.cc \
    # test/integration/TestString.cc \ @todo: reeanable string tests with string support
#

DOC_FILES := doc/doc.txt doc/language.txt doc/functions.txt doc/codeexamples.txt
DOC_INDEX := doc/html/index.html
DOC_PDF := doc/latex/refman.pdf

CMD_INCLUDE_NAMES := \
#

CMD_SRC_NAMES := \
    cmd/openvdb_ax/main.cc \
#


HEADER_SUBDIRS := $(dir $(INCLUDE_NAMES))

ALL_INCLUDE_FILES := \
    $(INCLUDE_NAMES) \
    $(TEST_INCLUDE_NAMES) \
    $(CMD_INCLUDE_NAMES) \
#
SRC_FILES := \
    $(SRC_NAMES) \
    $(TEST_SRC_NAMES) \
    $(CMD_SRC_NAMES) \
#
ALL_SRC_FILES := $(SRC_FILES)

OBJ_NAMES := $(SRC_NAMES:.cc=.o)
TEST_OBJ_NAMES := $(TEST_SRC_NAMES:.cc=.o)

LIB_MAJOR_VERSION=$(shell grep 'define OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER ' \
    $(OPENVDB_INCL_DIR)/openvdb/version.h | sed 's/[^0-9]*//g')
LIB_MINOR_VERSION=$(shell grep 'define OPENVDB_LIBRARY_MINOR_VERSION_NUMBER ' \
    $(OPENVDB_INCL_DIR)/openvdb/version.h | sed 's/[^0-9]*//g')
LIB_PATCH_VERSION=$(shell grep 'define OPENVDB_LIBRARY_PATCH_VERSION_NUMBER ' \
    $(OPENVDB_INCL_DIR)/openvdb/version.h | sed 's/[^0-9]*//g')

LIB_VERSION=$(LIB_MAJOR_VERSION).$(LIB_MINOR_VERSION).$(LIB_PATCH_VERSION)
SO_VERSION=$(LIB_MAJOR_VERSION).$(LIB_MINOR_VERSION)

LIBOPENVDB_AX_NAME=libopenvdb_ax
LIBOPENVDB_AX_STATIC := $(LIBOPENVDB_AX_NAME).a
ifndef MBSD
LIBOPENVDB_AX_SHARED_NAME := $(LIBOPENVDB_AX_NAME).so
LIBOPENVDB_AX_SHARED := $(LIBOPENVDB_AX_NAME).so.$(LIB_VERSION)
LIBOPENVDB_AX_SONAME := $(LIBOPENVDB_AX_NAME).so.$(SO_VERSION)
LIBOPENVDB_AX_SONAME_FLAGS := -Wl,-soname,$(LIBOPENVDB_AX_SONAME)
else
LIBOPENVDB_AX_SHARED_NAME := $(LIBOPENVDB_AX_NAME).dylib
LIBOPENVDB_AX_SHARED := $(LIBOPENVDB_AX_NAME).$(LIB_VERSION).dylib
LIBOPENVDB_AX_SONAME := $(LIBOPENVDB_AX_NAME).$(SO_VERSION).dylib
LIBOPENVDB_AX_SONAME_FLAGS := -Wl,-install_name,$(DESTDIR_LIB_DIR)/$(LIBOPENVDB_AX_SONAME)
endif

ifeq (no,$(strip $(shared)))
    LIBOPENVDB_AX := $(LIBOPENVDB_AX_STATIC)
else
    LIBOPENVDB_AX := $(LIBOPENVDB_AX_SHARED)
    LIBOPENVDB_AX_RPATH := -Wl,-rpath,$(DESTDIR_LIB_DIR)
endif # shared

DEPEND := dependencies

# Get the list of dependencies that are newer than the current target,
# but limit the list to at most three entries.
list_deps = $(if $(wordlist 4,5,$(?F)),$(firstword $(?F)) and others,$(wordlist 1,3,$(?F)))

ALL_PRODUCTS := \
    $(LIBOPENVDB_AX) \
    vdb_test \
    vdb_ax \
    $(DEPEND) \
    $(LIBOPENVDB_AX_SHARED_NAME) \
    $(LIBOPENVDB_AX_SONAME) \
#

.SUFFIXES: .o .cc

.PHONY: all clean depend .depend doc install lib pdfdoc test grammar

.cc.o:
	@echo "Building $@ because of $(call list_deps)"
	$(CXX) -c $(CXXFLAGS) -fPIC -o $@ $<

all: lib vdb_ax vdb_test depend

grammar:
	@echo "Rebuilding axlexer and axparser files"
	$(FLEX_BIN) -o grammar/axlexer.cc grammar/axlexer.l
	$(BISON_BIN) grammar/axparser.y --defines=grammar/axparser.h -o grammar/axparser.cc

grammar/axlexer.cc: grammar/axlexer.l
	$(FLEX_BIN) -o grammar/axlexer.cc grammar/axlexer.l

grammar/axparser.cc: grammar/axparser.y
	$(BISON_BIN) grammar/axparser.y --defines=grammar/axparser.h -o grammar/axparser.cc

$(OBJ_NAMES): %.o: %.cc
	@echo "Building $@ because of $(call list_deps)"
	$(CXX) -c -DOPENVDB_PRIVATE $(CXXFLAGS) -fPIC -o $@ $<

ifneq (no,$(strip $(shared)))

# Build shared library
lib: $(LIBOPENVDB_AX_SHARED_NAME) $(LIBOPENVDB_AX_SONAME)

$(LIBOPENVDB_AX_SHARED_NAME): $(LIBOPENVDB_AX_SHARED)
	ln -f -s $< $@

$(LIBOPENVDB_AX_SONAME): $(LIBOPENVDB_AX_SHARED)
	ln -f -s $< $@

$(LIBOPENVDB_AX_SHARED): $(OBJ_NAMES)
	@echo "Building $@ because of $(list_deps)"
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(LIBS_RPATH) $(LIBOPENVDB_AX_SONAME_FLAGS)

else

# Build static library
lib: $(LIBOPENVDB_AX)

$(LIBOPENVDB_AX_STATIC): $(OBJ_NAMES)
	@echo "Building $@ because of $(list_deps)"
	$(AR) cr $@ $^

endif # shared


$(DOC_INDEX): doxygen-config $(INCLUDE_NAMES) $(SRC_NAMES) $(DOC_FILES)
	@echo "Generating documentation because of $(list_deps)"
	echo 'OUTPUT_DIRECTORY=./doc' | cat doxygen-config - | $(DOXYGEN) - $(QUIET)

$(DOC_PDF): doxygen-config $(INCLUDE_NAMES) $(SRC_NAMES) $(DOC_FILES)
	@echo "Generating documentation because of $(list_deps)"
	echo -e 'OUTPUT_DIRECTORY=./doc\nGENERATE_LATEX=YES\nGENERATE_HTML=NO' \
		| cat doxygen-config - | $(DOXYGEN) - $(QUIET) \
		&& cd ./doc/latex && make refman.pdf $(QUIET) \
		&& echo 'Created doc/latex/refman.pdf'

ifneq (,$(strip $(DOXYGEN)))
doc: $(DOC_INDEX)
pdfdoc: $(DOC_PDF)
else
doc:
	@echo "$@"': $$DOXYGEN is undefined'
pdfdoc:
	@echo "$@"': $$DOXYGEN is undefined'
endif

vdb_ax: $(LIBOPENVDB_AX) cmd/openvdb_ax/main.cc
	@echo "Building $@ because of $(list_deps)"
	$(CXX) $(CXXFLAGS) -o $@ cmd/openvdb_ax/main.cc -I . \
		$(LIBOPENVDB_AX_RPATH) -L$(CURDIR) $(LIBOPENVDB_AX) \
		$(LIBS_RPATH) $(CONCURRENT_MALLOC_LIB)

$(TEST_OBJ_NAMES): %.o: %.cc
	@echo "Building $@ because of $(list_deps)"
	$(CXX) -c $(CXXFLAGS) -isystem $(CPPUNIT_INCL_DIR) -fPIC -o $@ $<

ifneq (,$(strip $(CPPUNIT_INCL_DIR)))
vdb_test: $(LIBOPENVDB_AX) $(TEST_OBJ_NAMES)
	@echo "Building $@ because of $(list_deps)"
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ_NAMES) \
		$(LIBOPENVDB_AX_RPATH) -L$(CURDIR) $(LIBOPENVDB_AX) \
		$(LIBS_RPATH) $(CONCURRENT_MALLOC_LIB) \
		-Wl,-rpath,$(CPPUNIT_LIB_DIR) -L$(CPPUNIT_LIB_DIR) $(CPPUNIT_LIB)

test: lib vdb_test
	@echo "Testing $(LIBOPENVDB_AX_NAME)"
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(CURDIR); ./vdb_test $(QUIET_TEST)
else
vdb_test:
	@echo "$@"': $$(CPPUNIT_INCL_DIR) is undefined'
test:
	@echo "$@"': $$(CPPUNIT_INCL_DIR) is undefined'
endif

install_lib: lib
	mkdir -p $(DESTDIR)/include/openvdb_ax
	@echo "Created $(DESTDIR)/include/openvdb_ax"
	pushd $(DESTDIR)/include/openvdb_ax > /dev/null; \
	    mkdir -p $(HEADER_SUBDIRS); popd > /dev/null
	for f in $(INCLUDE_NAMES); \
	    do cp -f $$f $(DESTDIR)/include/openvdb_ax/$$f; done
	@echo "Copied header files to $(DESTDIR)/include"
	@#
	mkdir -p $(DESTDIR_LIB_DIR)
	@echo "Created $(DESTDIR_LIB_DIR)"
	cp -f $(LIBOPENVDB_AX) $(DESTDIR_LIB_DIR)
	pushd $(DESTDIR_LIB_DIR) > /dev/null; \
	    if [ -f $(LIBOPENVDB_AX_SHARED) ]; then \
	        ln -f -s $(LIBOPENVDB_AX_SHARED) $(LIBOPENVDB_AX_SHARED_NAME); \
	        ln -f -s $(LIBOPENVDB_AX_SHARED) $(LIBOPENVDB_AX_SONAME); \
	    fi; \
	    popd > /dev/null
	@echo "Copied libopenvdb_ax to $(DESTDIR_LIB_DIR)"

install: install_lib vdb_ax doc
	mkdir -p $(DESTDIR)/bin
	@echo "Created $(DESTDIR)/bin/"
	cp -f vdb_ax $(DESTDIR)/bin
	@echo "Copied vdb_ax to $(DESTDIR)/bin/"
	if [ -d doc/html ]; \
	then \
		mkdir -p $(DESTDIR)/share/doc/openvdb_ax; \
		echo "Created $(DESTDIR)/share/doc/openvdb_ax/"; \
		cp -r -f doc/html $(DESTDIR)/share/doc/openvdb_ax; \
		echo "Copied documentation to $(DESTDIR)/share/doc/openvdb_ax/"; \
	fi

# TODO: This accumulates all source file dependencies into a single file
# containing a rule for each *.o file.  Consider generating a separate
# dependency file for each *.o file instead.

.depend: $(ALL_INCLUDE_FILES) $(ALL_SRC_FILES)
	@echo "Generating dependencies because of $(list_deps)"
	$(RM) $(DEPEND)
	for f in $(SRC_NAMES) $(CMD_SRC_NAMES); \
		do $(CXX) $(CXXFLAGS) -O0 \
			-MM $$f -MT `echo $$f | sed 's%\.[^.]*%.o%'` >> $(DEPEND); \
	done
	if [ -d "$(CPPUNIT_INCL_DIR)" ]; \
	then \
		for f in $(TEST_SRC_NAMES); \
			do $(CXX) $(CXXFLAGS) -O0 \
				-MM $$f -MT `echo $$f | sed 's%\.[^.]*%.o%'` \
				-isystem $(CPPUNIT_INCL_DIR) >> $(DEPEND); \
		done; \
	fi

depend: .depend

# Compile an implicit translation unit for each header to identify any indirect includes
HEADER_TEST_CXXFLAGS := $(CXXFLAGS)
HEADER_TEST_FILES := $(addprefix header_test-,$(INCLUDE_NAMES) $(CMD_INCLUDE_NAMES))
ifeq (yes,$(has_cppunit))
    HEADER_TEST_FILES += $(addprefix header_test-,$(TEST_INCLUDE_NAMES))
    HEADER_TEST_CXXFLAGS += -isystem $(CPPUNIT_INCL_DIR)
endif
$(HEADER_TEST_FILES): header_test-%:
	echo "#include \"$*\"" | $(CXX) -c -x c++ $(HEADER_TEST_CXXFLAGS) -fPIC -o /dev/null -
echo_header_test:
	@echo "Checking for missing or indirectly included headers"
header_test: echo_header_test $(HEADER_TEST_FILES)

clean:
	$(RM) $(OBJ_NAMES) $(ALL_PRODUCTS) $(DEPEND)
	$(RM) $(LIBOPENVDB_AX_STATIC)
	$(RM) $(LIBOPENVDB_AX_SHARED)
	$(RM) $(TEST_OBJ_NAMES)
	$(RM) -r ./doc/html ./doc/latex

ifneq (,$(strip $(wildcard $(DEPEND))))
    include $(DEPEND)
endif

# Copyright (c) 2015-2018 DNEG Visual Effects
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )

