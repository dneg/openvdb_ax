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

# Working Directory - /home/travis/build/dneg/openvdb_ax/

mkdir build
cd build

cmake \
    -D CMAKE_CXX_COMPILER=g++ \
    -D CMAKE_C_COMPILER=gcc \
    -D OPENVDB_BUILD_AX=ON \
    -D OPENVDB_BUILD_AX_DOCS=ON \
    -D OPENVDB_BUILD_AX_UNITTESTS=ON \
    -D OPENVDB_BUILD_AX_BINARIES=ON \
    -D OPENVDB_BUILD_AX_GRAMMAR=OFF \
    -D OPENVDB_BUILD_AX_PYTHON_MODULE=OFF \
    -D OPENVDB_CXX_STRICT=ON \
    ../

make -j2
echo "Installing openvdb_ax..."
sudo make install -j2 &>/dev/null

ctest -V
