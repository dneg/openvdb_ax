# OpenVDB AX [![ax](https://github.com/dneg/openvdb_ax/workflows/ax/badge.svg)](https://github.com/dneg/openvdb_ax/actions)

* [Introduction](#introduction)
* [Development Repository](#development-repository)
    * [License](#license)
    * [Documentation](#documentation)
    * [Requirements](#requirements)
    * [Developer Quick Start](#developer-quick-start)
* [Roadmap](#roadmap)


## Introduction

OpenVDB AX is an open source C++ library that provides a powerful and easy way of interacting with [OpenVDB](http://www.openvdb.org/) point and volume data. This exposes an expression language to allow fast, custom manipulation of point attributes and voxel values using a collection of mathematical functions. Expressions are quickly JIT-compiled and to offer great performance that in many cases rival custom C++ operators. It is developed and maintained by DNEG, providing a flexible and portable way to work with OpenVDB data.


## Development repository

This repository hosts the latest developments in the OpenVDB AX repository. This has been used in production at DNEG in various feature films yet remains a work-in-progress and such may be quite volatile. That is, both API and ABI **may be subject to change** so development using more niche features and lower-level components of the library (close to the LLVM component) should be approached with caution. However, use of the library and AX language (i.e. through the provided vdb_ax binary and Houdini OpenVDB AX SOP) is encouraged and future developments will focus on extending the language further to offer a wider range of functionality.

For the latest changes please see our [change log](CHANGES.md).


### License

OpenVDB AX is released under the [Mozilla Public License Version 2.0](https://www.mozilla.org/MPL/2.0/), which is a free, open source, and detailed software license developed and maintained by the Mozilla Foundation. It is a hybrid of the modified [BSD license](https://en.wikipedia.org/wiki/BSD_licenses#3-clause) and the [GNU General Public License](https://en.wikipedia.org/wiki/GNU_General_Public_License) (GPL) that seeks to balance the concerns of proprietary and open source developers. For more information see LICENSE.


### Requirements

OpenVDB AX follows the same requirements as OpenVDB. For more information on required dependency versions, see the [OpenVDB Dependency Page](https://www.openvdb.org/documentation/doxygen/dependencies.html). Importantly, OpenVDB AX requires the following:

 * A C++ 14 Compiler
 * [OpenVDB 6.2.1](https://github.com/AcademySoftwareFoundation/openvdb/releases/tag/v6.2.1) or later
 * [LLVM](https://llvm.org/). Though OpenVDB AX supports versions of LLVM >= 6.0, we recommend LLVM 8.0 which is the current version most used and tested against.

### Documentation

This library uses doxygen for creating its documentation. This documentation currently includes both developer and user documentation including language examples, syntax and list of available functions.

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

##### Building OpenVDB
```
git clone --branch v6.2.1 https://github.com/AcademySoftwareFoundation/openvdb.git openvdb
cd openvdb
mkdir build
cd build
cmake ..
make -j4
make install
```

##### Building OpenVDB AX
```
git clone https://github.com/dneg/openvdb_ax.git openvdb_ax
cd openvdb_ax
mkdir build
cd build
cmake ..
make -j4
make install
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
##### Building OpenVDB
```
git clone --branch v6.2.1 https://github.com/AcademySoftwareFoundation/openvdb.git openvdb
cd openvdb
mkdir build
cd build
cmake ..
make -j4
make install
```

##### Building OpenVDB AX
```
git clone https://github.com/dneg/openvdb_ax.git openvdb_ax
cd openvdb_ax
mkdir build
cd build
cmake ..
make -j4
make install
```

## Roadmap

Please see our [roadmap](ROADMAP.md) for more details.
