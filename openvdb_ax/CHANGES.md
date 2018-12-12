OpenVDB AX Version History
==========================

Version 0.0.4 - December 12, 2018

    Bug fixes:
    - Fixed a memory leak in Point and Volume Executable objects.
    - Fixed an issue with fit() which could use an incorrect precision for
      floating point arguments.
    - Compilation fixes for the Houdini AX SOP for Houdini 15.5 and earlier.

    Improvements:
    - Added a specialization for print for floats.

Version 0.0.3 - November 13, 2018

    New features:
    - Introduced new $ syntax and back-end logic for fast lookups into AX Custom
      Data. This syntax works identically to attribute access syntax with type
      specifiers expected to be present when accessing non float data. For
      literal path names, this can be up to x20 faster than using the lookup()
      functions.
    - New External Variable AST node, Code Generation/Visitor methods and
      internal intrinsic functions for processing Custom Data lookups using $
      syntax.
    - Added a --print-ast command to the vdb_ax binary to output the built AST
      from the provided code snippet.

    Improvements:
    - Removed the uint64_t LLVM Type specialization as all back-end values are
      signed.
    - Introduced a uintptr_t LLVM Type specialization for accessing pointers
      to typed data.
    - Consolidated attribute type checking with external variable type checking
      in Compiler.cc into verifyTypedAccesses().
    - Minor re-work to the PointDefaultModifier for conversion of default
      attributes.

    API Changes:
    - ax::print() now takes a const ast::Tree reference.

    Houdini:
    - External variable $ syntax can now be used to access parameters on the AX
      SOP e.g. ch("parm") -> $parm
    - Known path lookups using supported Houdini channel functions (e.g. ch())
      or AX lookup functions are optimized using the new external variable
      syntax for speed gains of up to x20.
    - Added optimised access to all available floating points Global and Local
      Node HScript variables such as $F, $T etc.
    - Back-ticks are no longer supported as the code is evaluated as a raw
      string. This is to support optimizations for ch() and Houdini HScript $
      parameters. These can be embedded without the need to re-compile the
      string parameter, even if they are time dependent.
    - Verbified the AX SOP so that it can be used in compile blocks and as a
      verb through python.


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
