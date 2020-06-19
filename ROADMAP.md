# OpenVDB AX Roadmap

This page holds the latest details on work in progress, completed goals and future plans for the development of OpenVDB AX. It is updated periodically as milestones develop.

## Completed goals

A list of completed features/milestones that previously appeared on the OpenVDB AX Roadmap

* Travis support for OpenVDB AX core library.
* Improved syntax and performance for the access of Custom Data (custom user parameters).
* CMake support for building OpenVDB AX on OSX.
* Support for string variables and string point attributes.
* Bison/Flex CMake Support for re-generating language grammar and lexer files.
* Noise functions supported by AX.
* Support for matrix variables and matrix point attributes.
* For/while loops.
* Ternary operators.

## Current goals

This section is dedicated to the near future and current milestones.

### v1.0.0

Most current work is aimed at our first stable release of OpenVDB AX. This is primarily defined by the lock down of the front end API which requires some refactoring of existing architecture before it is suitably future-proof. In addition to that we hope to include some other general improvements and features:

#### Improvements:
* Improved developer and user facing documentation.
* Additional tests.
* Better backend architecture for strings
* Solidifying the Compiler API and error/warning management

## Further down the road...

This section is where we talk about other longer term ambitions we have for OpenVDB AX. These generally take the shape of features to be added to the language. These are in no particular order and we are hoping for input from the community to help drive the direction we go with these.

* Support for array variables and array point attributes.
* User defined function declarations.
* Syntax highlighting for AX in Houdini and possibly some text editors.
* Voxel active state lookup and editing.
* Neighbouring voxel/point lookups.
* Windows CMake support and continuous integration.
* Sampling and rasterisation functionality.
* Travis and hython test support for OpenVDB AX Houdini
