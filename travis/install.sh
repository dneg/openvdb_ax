#!/bin/bash -e
#
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
# Builds OpenVDB AX
#
# Author: Matt Warner

echo "Building openvdb_ax..."

export ILMBASE_ROOT=/usr
export OPENEXR_ROOT=/usr
export BOOST_ROOT=/usr
export TBB_ROOT=/usr
export BLOSC_ROOT=/usr
export LLVM_ROOT=/usr
export CPPUNIT_ROOT=/usr
export OPENVDB_ROOT=/usr

mkdir build
cd build

# Temporarily change the /usr/bin/llvm-config symlink so CMake chooses
# the correct version of LLVM.
ORIGINAL_LLVM_CONFIG=`readlink /usr/bin/llvm-config`
sudo ln -sfn /usr/bin/llvm-config-5.0 /usr/bin/llvm-config

cmake \
    -D OPENVDB_ABI_VERSION_NUMBER=4 \
    -D MINIMUM_BOOST_VERSION=1.55 \
    -D OPENVDB_AX_BUILD_DOCS=ON \
    -D OPENVDB_AX_BUILD_UNITTESTS=ON \
    -D ILMBASE_NAMESPACE_VERSIONING=OFF \
    ../

make -j2

echo "Installing openvdb_ax..."

sudo make install -j2 &>/dev/null

# Tests require running from the same root as the test snippets.
cd openvdb_ax/test

# Don't use ctest as we don't get much info even with verbose flags.
./vdb_ax_test -v

# Reset the symlink for /usr/bin/llvm-config to original setting.
sudo ln -sfn $ORIGINAL_LLVM_CONFIG /usr/bin/llvm-config
