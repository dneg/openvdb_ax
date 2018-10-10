OpenVDB AX Version History
==========================

Version 0.0.2 - October 8, 2018

    Improvements:
    - Added library version naming for libopenvdb_ax.
    - Updated Compute Generator initialization methods to leverage the
      function signature framework.
    - Improved the usage of CustomData for runtime retrieval of user parameters.
    - Add Travis CI support for the core library.
    - A variety of comment fixes and improvements.
    - Added COPYRIGHT and CHANGES file.

    Bug fixes:
    - Compilation fixes to the vdb_ax command-line binary.
      [Reported by Elisa Prana]
    - Fixed an issue where LLVM would assert due to void pointer instantiation.
      [Reported by Elisa Prana]
    - Fixed an issue where MCJIT was not being linked in on some platforms.
      [Reported by Elisa Prana]
    - CMake and build fixes for Houdini 17.

    API Changes:
    - Refactoring of the Compute Functions and their usage within their
      corresponding Executables. Moved a portion of compiler specific code
      into the compiler directory and out of codegen.
    - Moved LeafLocalData into the compiler directory.
    - Added m prefix to members of CompilerOptions.
    - Added a constant shared pointer typedef to CustomData.
    - Removed the getFunctionList() compute generator function.
    - The CustomData argument for Executable::compiler now has a default empty
      argument and is optional.
    - Removed CustomData argument from the ComputeGenerator class constructors.


Version 0.0.1 - August 10 2018 (initial public release)
