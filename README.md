# OpenVDB AX

* [Migration](#migration)
* [Introduction](#introduction)
* [License](#license)

## Migration

### **This repository is in maintenance mode.**

As of OpenVDB AX 1.0.0 and OpenVDB 8.0.0, this repository has been merged into the [OpenVDB ASWF repository](https://github.com/AcademySoftwareFoundation/openvdb). All future development for OpenVDB AX will be focused there. It is strongly encouraged to use OpenVDB AX through the OpenVDB repository, which contains fixes, improvements and up to date documentation!

See https://github.com/AcademySoftwareFoundation/openvdb


## Introduction

OpenVDB AX is an open source C++ library that provides a powerful and easy way of interacting with [OpenVDB](http://www.openvdb.org/) point and volume data. This exposes an expression language to allow fast, custom manipulation of point attributes and voxel values using a collection of mathematical functions. Expressions are quickly JIT-compiled and to offer great performance that in many cases rival custom C++ operators. It was originally developed and maintained by DNEG, providing a flexible and portable way to work with OpenVDB data.


### License

OpenVDB AX is released under the [Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/), which is a free, open source, and detailed software license developed and maintained by the Mozilla Foundation. It is a hybrid of the modified [BSD license](https://en.wikipedia.org/wiki/BSD_licenses#3-clause) and the [GNU General Public License](https://en.wikipedia.org/wiki/GNU_General_Public_License) (GPL) that seeks to balance the concerns of proprietary and open source developers. For more information see LICENSE.


### Requirements

 * A C++ 14 Compiler
 * [OpenVDB 7.0.0](https://github.com/AcademySoftwareFoundation/openvdb/releases/tag/v7.0.0) or later
 * [LLVM](https://llvm.org/). Though OpenVDB AX supports versions of LLVM >= 6.0, we recommend LLVM 8.0 which is the current version most used and tested against.


### Developer Quick Start

#### Linux
##### Installing Dependencies (Boost, TBB, OpenEXR, Blosc)

```
apt-get install -y cmake
apt-get install -y doxygen
apt-get install -y libboost-iostreams-dev
apt-get install -y libboost-system-dev
apt-get install -y libboost-thread-dev
apt-get install -y libcppunit-dev
apt-get install -y libghc-zlib-dev
apt-get install -y libtbb-dev
apt-get install -y libblosc-dev
apt-get install -y libedit-dev
apt-get install -y llvm-8-dev
```
```
git clone git@github.com:Blosc/c-blosc.git
cd c-blosc
git checkout tags/v1.5.0 -b v1.5.0
mkdir build
cd build
cmake ..
make -j4
make install
cd ../..
```

#### macOS
##### Installing Dependencies (Boost, TBB, OpenEXR, Blosc)
```
brew install ilmbase;
brew install openexr;
brew install cmake;
brew install doxygen;
brew install boost;
brew install cppunit;
brew install c-blosc;
brew install tbb;
brew install llvm@8;
brew install zlib;
```
```
git clone git@github.com:Blosc/c-blosc.git
cd c-blosc
git checkout tags/v1.5.0 -b v1.5.0
mkdir build
cd build
cmake ..
make -j4
make install
cd ../..
```

#### Building OpenVDB
```
git clone https://github.com/AcademySoftwareFoundation/openvdb.git openvdb
cd openvdb
mkdir build
cd build
cmake ..
make -j4
make install
```

#### Building OpenVDB AX
```
git clone https://github.com/dneg/openvdb_ax.git openvdb_ax
cd openvdb_ax
mkdir build
cd build
cmake ..
make -j4
make install
```
