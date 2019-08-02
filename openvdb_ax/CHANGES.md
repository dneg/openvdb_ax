OpenVDB AX Version History
==========================

Version 0.1.0 - July 28, 2019

    New Features:
    - Support for for, while and do-while loops using standard c-like syntax.
      Includes break and continue keyword handling.
    - Attribute, external and local support for 3x3 and 4x4 matrices and 2 and 4
      component vectors using the following tokens: mat3f, mat3d, mat4f, mat4d,
      vec2i, vec2f, vec2d, vec4i, vec4f, vec4d. Attributes and externals use the
      same tokens with @ and $ respectively
    - New array style indexing support for vectors and matrices. Both can be
      indexed as an array using single [0] operations. Matrices can be indexed
      by row, column using [0,0]
    - Added non keyword {} scope support
    - New functions added: identity3, identity4, postscale, prescale,
      pretransform, simplexnoise, transform, transpose, curlsimplexnoise
    - Significantly improved the performance of duplicate attribute accesses for
      points and volumes
    - New compound assignment support for %=, &=, |= and ^=
    - Better handling of LHS values within assignment operations, including
      support for cremented attributes
    - Added pyopenvdbax module which exposes basic python bindings for AX.
      Provides compilation of executables and execution on VDB grids through
      Python.
    - New arguments and modes to the vdb_ax binary. Analysis provides detailed
      information for a given ax snippet. More tools to search and list available
      functions
    - Significantly re-worked volume execution. Each volume now receives the
      same singularly generated AX function. This more closely matches Houdini
      VEX behaviour
    - New reprint() methods for printing an AST as AX code
    - New scanner methods and API for returning detailed information from an AST
    - Added the ability to iterate over any active state for Volumes

    Bug fixes:
    - Fixed a crash which could occur when destructing AX executables. This was
      due to the destruction order of LLVM objects which have since been
      reversed.
    - Fixed a bug in SymbolTableBlocks::find which could occur if an out of range
      index was used
    - Fixed an issue during AX compilation caused by invalid IR when branching
      after a return statement

    Improvements:
    - Function signatures are now printed with their AX types rather than their
      LLVM types (i.e. int(short) vs i32(i16))
    - Significantly improved the descriptive printing of any parsed AST
    - Added lexer location tracking for error line and column numbering
    - Significantly improved backend string support
    - Introduced new methods for dependency tracking attributes for efficiently
      determining the copy requirements of volumes
    - Moved testing CMake config into its own CMakeLists.txt.
    - Significantly improved the descriptive printing of any parsed AST
    - Improved code documentation throughout the library
    - Improved doxygen generation
    - Improved position access for OpenVDB Points
    - Renamed 'lookup' function to 'external'
    - Moved function structs to .cc's and added methods to populate function
      registries
    - Removed the old FunctionContext System from derived function methods
    - Removed legacy code supporting multiple return values from
      FunctionSignatures
    - Added virtual destructors to FunctionBase and FunctionSignature classes

    Houdini:
    - Fixed an issue where the VDB name was taken over the Houdini Primitive
      name for VDBs passed into the OpenVDB AX SOP
    - Renamed the OpenVDB AX node type from DN_OpenVDBAX to openvdbax
    - Improved the OpenVDB AX SOP UI, added the ability to iterate over
      different value states and the ability to post compact point attributes

    AST:
    - The Visitor and Modifier have been removed in favour of a new Visitor CRTP
    - Introduced AST tokens for type inference
    - Introduced implicit parent tracking to AST nodes
    - Introduced full and short AST node names
    - Converted all child AST nodes to unique pointers from shared pointers
    - Introduced child indexing and in-place node replacement
    - New token to represent types to avoid passing around strings
    - Removed VectorPack/Unpack in favour of ArrayPack/Unpack
    - Refactored the crement node to only own a single variable

    Grammar:
    - Significant clean-up and improvements to lexer and parser
    - Removed legacy component assignments in favour of new workflows for point
      and volume vector attributes in their respective code generators
    - mat3f, mat3d, mat4f, mat4d, vec2i, vec2f, vec2d, vec4i, vec4f, vec4d, []
      and [,] support
    - Improved the operator precedence definitions in the parser
    - New variable_reference rule for assignable objects
    - Types passed around using new type tokens
    - Bison and Flex methods are now all prefixed (with ax) to support multiple
      lexers within the same program
    - Removed unused tokens in the lexer
    - Improved lexer whitespace support
    - Added reserved keywords in the lexer

    Code Generation:
    - Strings are now represented by a custom LLVM struct which holds a
      re-allocatable char* and an int64_t for the size
    - Getters and setters for new attribute type support
    - All literals (except for arrays) are passed around as constants and are no
      longer allocated
    - String literals are now created as global IR values so that the same
      literals can be shared
    - Removed VectorPack/Unpack IR generation in favour of ArrayPack/Unpack
    - Updated all visitors to use new AST methods
    - Point/volume compute generator no longer overrides crement and assign.
      Instead, attributes are retrieved once at the beginning of the tree and
      set at the end
    - Point/volume compute generator create and return their own attribute
      registry to infer read/write accesses
    - The volume generator now accepts an additional accessor and index
      representing the volume being accessed
    - The function framework now supports nullptr returns for IR functions which
      return void
    - Changed the core function storage from void* to void(*)() to adhere to
      point-to-function standard

    Compiler:
    - Completely removed volume blocks. A single function is generated which is
      run for every volume
    - Volume executable updated to support new volume workflow

    CMake / Build / Testing:
    - Updated to OpenVDB 6.1 CMake modules
    - Added CMake support for grammar generation
    - Added CMake options for static/shared library generation
    - Updated the CMake test framework to run the snippets directly from the
      source repository without copying
    - Improved LLVM version support for versions 6/7/8
    - Various fixes for clang and MacOS
    - Overhaul of frontend tests to compare full AST generation
    - Travis improvements including OSX builds
    - Travis build matrices for LLVM 5/6/7/8, clang/gcc, osx/linux

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
