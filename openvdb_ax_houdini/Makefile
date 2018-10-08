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
# Makefile for OpenVDB AX Houdini DSO
#
# This makefile assumes that you are in a Houdini environment,
# i.e., that you have sourced the houdini_setup script.
#

# Targets:
#   houdinisops         Houdini OpenVDB AX SOP (installed into HOUDINI_DESTDIR)
#
#   all                 [default target] all of the above
#   install             install the SOPs into HOUDINI_DESTDIR
#   clean               delete generated files from the local directory
#   depend              recompute source file header dependencies
#
# Options:
#   abi=N               build for compatibility with version N of the
#                       OpenVDB Grid ABI, where N is 2, 3, 4, etc.
#                       (some newer features will be disabled)
#   shared=no           link executables against static OpenVDB libraries
#                       (default: link against shared libraries)
#   debug=yes           build with debugging symbols and without optimization
#   verbose=yes         run commands (in particular, hcustom and doxygen)
#                       in verbose mode


#
# The following variables must be defined, either here or on the command line
# (e.g., "make install DESTDIR=/usr/local"):
#
# Note that the OpenVDB, OpenVDB Houdini & OpenVDBAX libraries and these tools must be built
# against compatible versions of the Boost, OpenEXR and TBB libraries.
# Until Houdini 16.5, all three were included in the HDK, so the relevant
# variables below point by default to the HDK library and header directories:
# $(HDSO) and $(HT)/include, respectively.  (Source the houdini_setup script
# to set those two environment variables.)  Beginning with Houdini 16.5,
# you must provide your own distribution of Boost.
#

ifeq (,$(HFS))
    $(warning Warning: $$(HFS) is not set; did you source houdini_setup?)
endif

# The directory into which to install libraries and header files
# (it's recommended that this match the DESTDIR in the OpenVDB
# library makefile)
DESTDIR := /tmp/OpenVDB

# The directory into which to install libraries (e.g., for Linux multiarch support)
DESTDIR_LIB_DIR := $(DESTDIR)/lib

# The directory into which to install Houdini SOPs
HOUDINI_DESTDIR := $(HIH)/dso

# The parent directory of the openvdb/ header directory
OPENVDB_INCL_DIR := $(DESTDIR)/include

# The directory containing libopenvdb
OPENVDB_LIB_DIR := $(DESTDIR_LIB_DIR)
OPENVDB_LIB := -lopenvdb

# The parent directory of the openvdb/ header directory
OPENVDB_AX_INCL_DIR := $(DESTDIR)/include
# The directory containing libopenvdb
OPENVDB_AX_LIB_DIR := $(DESTDIR)/lib
OPENVDB_AX_LIB := -lopenvdb_ax

# The parent directory of the boost/ header directory
BOOST_INCL_DIR := $(HT)/include
# The directory containing libboost_iostreams, libboost_system, etc.
BOOST_LIB_DIR := $(HDSO)
BOOST_LIB := -lboost_iostreams -lboost_system

# The parent directory of the OpenEXR/ header directory (which contains half.h)
ILMBASE_INCL_DIR := $(HT)/include
# The directory containing libHalf
ILMBASE_LIB_DIR := $(HDSO)
HALF_LIB := -lHalf

# The parent directory of the tbb/ header directory
TBB_INCL_DIR := $(HT)/include
# The directory containing libtbb
TBB_LIB_DIR := $(HDSO)
TBB_LIB ?= -ltbb

# The LLVM directory containing bin, lib, include, etc.
LLVM_ROOT := /apps/Linux64/llvm/llvm-3.4
LLVM_INCL_DIR := $(LLVM_ROOT)/include

#
# Ideally, users shouldn't need to change anything below this line.
#

SHELL = /bin/bash


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

HOUDINI_INCL_DIR := $(HT)/include
# Flags to be passed to the hcustom build script
HCUSTOM_FLAGS := -i $(HOUDINI_DESTDIR)
ifeq (yes,$(strip $(verbose)))
    HCUSTOM_FLAGS += -e
endif
# Extra flags to be passed through hcustom to the compiler or linker
HCUSTOM_EXTRA_CFLAGS ?= -DIGNORE_HDK_DEPRECATIONS
HCUSTOM_EXTRA_LDFLAGS :=
# The compiler to be used by hcustom
ifeq ($(strip $(HOUDINI_CC)),)
    # If HOUDINI_CC is not set, set it to CXX if CXX is set, otherwise set it to "g++".
    # (This replicates the logic in hcustom.)
    ifneq ($(strip $(CXX)),)
        HOUDINI_CC = $(CXX)
    else
        HOUDINI_CC = g++
    endif
endif

ifeq (yes,$(strip $(debug)))
    OPTIMIZE := -g
    HCUSTOM_FLAGS += -g
else
    OPTIMIZE := -O3 -DNDEBUG
    HCUSTOM_EXTRA_CFLAGS += -DNDEBUG
endif

ifeq (no,$(strip $(verbose)))
	QUIET := > /dev/null
else
    QUIET :=
endif

INCLDIRS := -I . -I ./ax -I $(OPENVDB_INCL_DIR) -I $(OPENVDB_AX_INCL_DIR) -isystem $(BOOST_INCL_DIR) -isystem $(ILMBASE_INCL_DIR) -isystem $(TBB_INCL_DIR)
INCLDIRS += -isystem $(LLVM_INCL_DIR)
HCUSTOM_INCLDIRS := $(subst isystem,I,$(INCLDIRS))

# add llvm stuff
# adding include dirs here again twice - shouldn't be a problem
HCUSTOM_EXTRA_CFLAGS += $(shell $(LLVM_ROOT)/bin/llvm-config --cppflags)

CXXFLAGS += -std=c++11

CXXFLAGS += -pthread $(OPTIMIZE) $(INCLDIRS) -DIGNORE_HDK_DEPRECATIONS
abi := $(strip $(abi))
ifneq (,$(abi))
    CXXFLAGS += -DOPENVDB_3_ABI_COMPATIBLE  # TODO: deprecated
    CXXFLAGS += -DOPENVDB_ABI_VERSION_NUMBER=$(abi)
    override HCUSTOM_EXTRA_CFLAGS += -DOPENVDB_3_ABI_COMPATIBLE  # TODO: deprecated
    override HCUSTOM_EXTRA_CFLAGS += -DOPENVDB_ABI_VERSION_NUMBER=$(abi)
endif
ifdef MBSD
    # Darwin ld treats undefined symbols as errors by default;
    # change to runtime resolution, like Linux.
    CXXFLAGS += -undefined dynamic_lookup
endif

ifneq (,$(BLOSC_INCL_DIR))
    CXXFLAGS += -DOPENVDB_USE_BLOSC
    HCUSTOM_EXTRA_CFLAGS += -DOPENVDB_USE_BLOSC
endif

ifneq ($(strip $(wildcard $(HOUDINI_INCL_DIR))),)
    HOUDINI_CXXFLAGS := $(shell hcustom -c)
    HOUDINI_LDFLAGS := $(shell hcustom -m) -lHoudiniOP2 -lHoudiniAPPS3
endif

# add LLVM stuff
CXXFLAGS += $(shell $(LLVM_ROOT)/bin/llvm-config --cppflags)

LIBS := \
    -ldl -lm -lz \
    -L$(HALF_LIB_DIR) $(HALF_LIB) \
    -L$(TBB_LIB_DIR) $(TBB_LIB) \
    -L$(BOOST_LIB_DIR) $(BOOST_LIB) \
    -L$(OPENVDB_LIB_DIR) $(OPENVDB_LIB) \
    -L$(OPENVDB_AX_LIB_DIR) $(OPENVDB_AX_LIB) \
#
LIBS_RPATH := \
    -ldl -lm -lz \
    -Wl,-rpath,$(ILMBASE_LIB_DIR) -L$(ILMBASE_LIB_DIR) $(HALF_LIB) \
    -Wl,-rpath,$(TBB_LIB_DIR) -L$(TBB_LIB_DIR) $(TBB_LIB) \
    -Wl,-rpath,$(BOOST_LIB_DIR) -L$(BOOST_LIB_DIR) $(BOOST_LIB) \
#
ifdef LINUX
    LIBS += -lrt
    LIBS_RPATH += -lrt
endif

HOUDINI_SOP_INCL_NAMES := \
    ax/HoudiniAXUtils.h \
#

HOUDINI_SOP_SRC_NAMES := \
    SOP_OpenVDB_AX.cc \
#

#
HOUDINI_SOP_NAMES := $(basename $(notdir $(HOUDINI_SOP_SRC_NAMES)))

#
#

ALL_INCLUDE_FILES := \
    $(HOUDINI_SOP_INCL_NAMES) \
#
HOUDINI_SRC_FILES := \
    $(HOUDINI_SOP_SRC_NAMES) \
#
ALL_SRC_FILES := $(HOUDINI_SRC_FILES)


HOUDINI_SOP_OBJ_NAMES := $(HOUDINI_SOP_SRC_NAMES:.cc=.o)
HOUDINI_SOP_SO_NAMES := $(addprefix $(HOUDINI_DESTDIR)/,\
    $(notdir $(HOUDINI_SOP_SRC_NAMES:.cc=.so)))

DEPEND := dependencies

# Get the list of dependencies that are newer than the current target,
# but limit the list to at most three entries.
list_deps = $(if $(wordlist 4,5,$(?F)),$(firstword $(?F)) and others,$(wordlist 1,3,$(?F)))

.SUFFIXES: .o .cc

.PHONY: all clean depend houdinisops install

.cc.o:
	@echo "Building $@ because of $(call list_deps)"
	$(CXX) -c $(CXXFLAGS) -fPIC -o $@ $<

all: depend houdinisops

# -isystem $(OPENVDB_INCL_DIR)

ifeq (no,$(strip $(shared)))
# Link SOPs against static OpenVDB libraries.
$(HOUDINI_SOP_OBJ_NAMES): %.o: %.cc
	@echo "Building $(HOUDINI_DESTDIR)/$(@:.o=.so) because of $(call list_deps)"
	@# TODO: This seems to be the only way to pass extra compiler flags to hcustom.
	/bin/env HOUDINI_CC='$(HOUDINI_CC) $(HCUSTOM_EXTRA_CFLAGS) $(HCUSTOM_EXTRA_LDFLAGS)' \
	    $(HB)/hcustom $(HCUSTOM_FLAGS) $(HCUSTOM_INCLDIRS) $(LIBS) \
	        -L. -lopenvdb_houdini \
		-L$(OPENVDB_LIB_DIR) $(OPENVDB_LIB) -L$(OPENVDB_AX_LIB_DIR) $(OPENVDB_AX_LIB) $(HOUDINI_LDFLAGS) $< $(QUIET)
else
# Link SOPs against shared OpenVDB libraries.
$(HOUDINI_SOP_OBJ_NAMES): %.o: %.cc
	@echo "Building $(HOUDINI_DESTDIR)/$(@:.o=.so) because of $(call list_deps)"
	@# TODO: This seems to be the only way to pass extra compiler flags to hcustom.
	/bin/env HOUDINI_CC='$(HOUDINI_CC) $(HCUSTOM_EXTRA_CFLAGS) $(HCUSTOM_EXTRA_LDFLAGS)' \
	    $(HB)/hcustom $(HCUSTOM_FLAGS) $(HCUSTOM_INCLDIRS) \
	        -L. -lopenvdb_houdini  \
	        -L$(OPENVDB_LIB_DIR) $(OPENVDB_LIB) -L$(OPENVDB_AX_LIB_DIR) $(OPENVDB_AX_LIB) $(HOUDINI_LDFLAGS) $< $(QUIET)
endif

$(HOUDINI_SOP_SO_NAMES): $(HOUDINI_DESTDIR)/%.so: %.o

houdinisops: $(HOUDINI_SOP_SO_NAMES)

install: houdinisops
	mkdir -p $(DESTDIR)/include/openvdb_ax_houdini
	@echo "Created $(DESTDIR)/include/openvdb_ax_houdini"
	cp -f $(HOUDINI_SOP_INCL_NAMES) $(DESTDIR)/include/openvdb_ax_houdini/

# TODO: This accumulates all source file dependencies into a single file
# containing a rule for each *.o file.  Consider generating a separate
# dependency file for each *.o file instead.
depend: $(ALL_INCLUDE_FILES) $(ALL_SRC_FILES)
	@echo "Generating dependencies because of $(call list_deps)"
	$(RM) $(DEPEND)
	for f in $(HOUDINI_SRC_FILES); \
		do $(CXX) $(CXXFLAGS) $(HOUDINI_CXXFLAGS) -O0 \
			-MM $$f -MT `echo $$f | sed 's%\.[^.]*%.o%'` \
			-isystem $(HOUDINI_INCL_DIR) >> $(DEPEND); \
	done

clean:
	$(RM) $(DEPEND)
	$(RM) $(HOUDINI_SOP_OBJ_NAMES) $(HOUDINI_SOP_SO_NAMES)

ifneq ($(strip $(wildcard $(DEPEND))),)
    include $(DEPEND)
endif

# Copyright (c) 2015-2018 DNEG Visual Effects
# All rights reserved. This software is distributed under the
# Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
