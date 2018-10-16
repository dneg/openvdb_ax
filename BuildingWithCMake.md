# Building with CMake:

The CMake build infrastructure for OpenVDB AX is designed for out-of-source build. This is to avoid overwriting the existing Makefile that already exists in the source tree.
For a list of dependencies of the OpenVDB AX library see INSTALL, there you will also find instructions to build with Make. This document will give alternative instructions to allow building with CMake.

The supplied CMake builder currently requires the setting of a collection of environment variables for:

- ILMBASE_ROOT
- OPENEXR_ROOT
- BOOST_ROOT
- TBB_ROOT
- BLOSC_ROOT
- LLVM_ROOT
- CPPUNIT_ROOT
- OPENVDB_ROOT
- OPENVDB_HOUDINI_ROOT (used in building the Houdini SOP)
- OPENVDB_AX_ROOT (used in building the Houdini SOP, when not building the AX library)
- HDK_LOCATION (used in building the Houdini SOP, optional)

Alternatively to providing HDK_LOCATION for building the Houdini SOP you can also source the houdini_setup script to correctly setup your environment.

Once these are set, make a temporary directory and from there invoke CMake i.e.

```
mkdir build
cd build
cmake \
    -D OPENVDB_ABI_VERSION_NUMBER=4 \
    -D MINIMUM_BOOST_VERSION=1.55 \
    -D ILMBASE_NAMESPACE_VERSIONING=OFF \
    -D CMAKE_INSTALL_PREFIX=[INSTALL DIRECTORY] \
    ../
```
(replacing [INSTALL DIRECTORY] with your preferred install location, and making sure to use the OpenVDB ABI number of your OpenVDB install.)

Use the options -D OPENVDB_AX_BUILD_AX or OPENVDB_BUILD_AX_HOUDINI_SOP=ON/OFF to choose building the library or the SOP only. The SOP will require the AX library to be installed at OPENVDB_AX_ROOT if the library is not being built too.

Then make and install the library (or SOP depending on your chosen options in the top level CMakeLists.txt) with:

```
make install -j32

```

Thanks to those who supplied the CMake setup in OpenVDB upon which this is heavily based.
