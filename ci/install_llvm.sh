#!/usr/bin/env bash

set -ex

# Download, build and install LLVM from source

LLVM_VERSION=$1
LLVM_ROOT_DIR=$2
if [ -z $LLVM_VERSION ]; then
    echo "No LLVM version provided for LLVM installation"
    exit -1
fi
if [ -z $LLVM_ROOT_DIR ]; then
    echo "No installation directory provided for LLVM installation"
    exit -1
fi

echo "Downloading LLVM $LLVM_VERSION src..."

wget -O llvm-$LLVM_VERSION.src.tar.xz https://releases.llvm.org/$LLVM_VERSION/llvm-$LLVM_VERSION.src.tar.xz
tar -xf llvm-$LLVM_VERSION.src.tar.xz
rm -f llvm-$LLVM_VERSION.src.tar.xz
cd llvm-$LLVM_VERSION.src

echo "Building LLVM $LLVM_VERSION -> $LLVM_ROOT_DIR ..."

mkdir -p .build
cd .build
cmake -DCMAKE_INSTALL_PREFIX=$LLVM_ROOT_DIR -DCMAKE_BUILD_TYPE=Release ../
make -j2
make install
