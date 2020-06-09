OpenVDB AX Version History
==========================

Version 1.0.0 - In Development

    New Features:
    - New and improved doxygen and language reference material. NOTE: Work on
      this is still in progress. Incremental changes will continue to be made
      up to release.
    - Added ternary conditional operator 'a ? b : c' (and "Elvis" operator
      'a ?: c') support
    - Added full AX type support for matching VDB Point/Volume types which may
      or may not be registered. Registration of these types still depends on
      on downstream software.

    Improvements:
    - Updated scalar to matrix promotion rules. When a scalar is promoted to a
      matrix (either through direct/indirect assignments, binary or ternary
      operators) it now produces the diagonal matrix with an equivalent
      dimension with components equal to the scalar. For example "mat3d@a = 2.0;"
      produces { 2.0,0.0,0.0  0.0,2.0,0.0  0.0,0.0,2.0 }
    - polardecompose() no longer throws on unsuccessful inversion and will now
      return false if it fails.
    - Removed the ability to compare vectors or matrices with the following
      relation operators: <= >= < >
    - Removed support for implicit narrowing of vector/matrices to scalars
      i.e. vec3f a; int b = a;
    - Removed support for incrementing or decrementing boolean values.
    - Added support for unary logical/bitwise not on integer vector types
      e.g. vec3i a; a = !a;
    - Implemented complete support for the comma operator e.g. int a = (1,2,3);
      which evaluates all expressions in order, returning the last expression's
      value.
    - Compound assignments no longer evaluate the left hand side twice.
    - Added the ability to assign to precremented values i.e. ++a = 1;
    - Added API support for VDB 7.1.0.

    Bug Fixes:
    - Fixed a bug which would cause a crash if a vector/matrix was accessed
      with a single index consisting of an invalid type. e.g. mat[{1,1,1}];
    - Fixed parser issues related to parenthesized comma operators. This
      primarily caused a parser failure when calling a function with a
      parenthesized expression e.g. print((1,2));
    - Fixed some attribute dependency detections that would miss dependencies
      in conditions of loops.
    - Fixed a variety of boolean conversion logic in binary, unary, crement
      and direct/indirect assignments. Boolean conversion from scalars now
      adheres to C/C++ standard.
    - Fixed a potential bug which could cause a crash if the target machine type
      could not be inferred by LLVM.
    - Fixed a small memory leak which would occur when invalid code was parsed
    - Fixed issues with the way declaration lists were being printed with
      ast::reprint in loop initializers

    Houdini:
    - The AX SOP automatically drops unsupported grid types with a warning.

    AST:
    - New AST Nodes for Comma and Ternary operators and corresponding visitor
      patterns.
    - Removed the ExpressionList AST node which didn't represent a
      syntactical structure within AX. Such expressions are now either
      arguments or Comma operators
    - Renamed the then and else branch methods on the CondtionalStatement AST
      node to true and false respectively. This matches the new Ternary AST
      node.
    - Reordered the Binary and Unary Operator AST node constructors to always
      take the AST node operands first. This better matches other AST nodes.
    - Restructured the Assign AST node to better handle compound assignments.
      The node no longer necessarily stores a binary expression on the right
      hand side to represent the compound assignment and instead takes an
      optional binary token.
    - Swapped the right and left order of operands on the Assign AST node
      to better match other AST nodes. This also changes default traversal
      order.
    - FunctionCall, ArrayUnpack and ArrayPack AST nodes no longer store
      ExpressionList nodes and instead directly store the AST nodes
      representing their arguments.
    - Restructured Declaration AST nodes. These are no longer derived
      variables and are instead Statements with Local AST composition.
      Optional initializer expressions can also be stored on declarations
      instead of the previous nesting within AssignExpressions.
    - Improved the result of ast::reprint in regards to scoped blocks
      consistency.
    - Removed the ast::variableDependencies method from the public API.

    Grammar:
    - Changes to array packs where arguments are stored in a temporary vector
      of nodes until the parsing context can be inferred. This avoid conflicts
      with scopes.
    - Split function call grammar into two rules which append arguments.
    - Changes to declarations to support their new context as statements.
    - Changes to assign expressions such that compound expressions directly
      store the right hand side instead of building additional binary
      expressions.
    - Constructor changes in binary and unary grammar rules to match AST.
    - New ternary grammar rules. Support for ? and : tokens in the lexer.
    - Added destructor directives to handle cleanup of bad symbols.

    Code Generation:
    - Converted static methods on the ArgumentIterator class to a generic
      apply method
    - Support for Comma and Ternary Operator code generation.
    - Split out the assignment and binary logic into protected methods. Declare
      visits use the assignment logic, compound assignments use the binary
      logic.

    CMake / Build / Testing:
    - Added github actions CI for VFX platforms 2019 and 2020 which match
      existing travis functionality
    - Bison now errors on any warnings during grammar generation.
    - Updated CMake use of llvm_map_components_to_libnames to support LLVM 10.
    - Added matrix grid detection during the CMake build step.
    - Fixed TestLoopNode tests which were not being executed
    - Started re-factor of auto-generated integration tests and considerably
      improved test coverage. AX tests can be regenerated with the -g argument
      to the unit test binary.

Version 0.2.0 - March 5, 2020

    As of this release, a C++14 compiler is required.

    New Features:
    - AX now generates all target machine (host) optimizations by default. This
      includes any supported SIMD (sse, avx etc.) instructions for SLP
      vectorization
    - A full restructure of the developer Function framework with the switch to
      IR functions implementations over C bindings (where available). Up until
      now, natively supported functions in AX have used their C bindings by
      default. This release defaults to their IR implementations which, along
      with full vectorization support, allows for the full potential of all LLVM
      optimizations. This is most noticeable when using function inside of loops
      or in larger AX code snippets
    - New functions added: acosh, asinh, atanh, diag
    - New options and output to the vdb_ax binary. --try-compile provides a way
      to test if a given AX code snippet will compile for a particular VDB
      target. Verbose mode with -v now provides better formatted output with
      more detailed timing reports.
    - Added the ability to provide external string data to AX which can be
      accessed with string$ (or chs() in Houdini)
    - Added support for vec4 and string VDB volumes. Note that vec4 volumes are
      not registered by default

    Improvements:
    - Significant performance improvements to AX execution with the introduction of:
        - Proper SLP vectorization with supported host features
        - Defaulting IR functions to ON
        - Loop allocation fixes (see bug fixes)
        - Constant Folding for scalar C bindings
    - Introduced initial constant folding for scalar C bindings. Functions which
      are called with known constants are evaluated at compile time. Note that
      CF for IR Functions is left to LLVM's optimization passes
    - Added support for vec2 and vec4 arguments for length() and lengthsq()
    - Added a new floating point signature for abs() (which calls fabs) to avoid
      implicit casting of integer arguments

    Bug Fixes:
    - Fixed a bug where assignments from functions which return void could cause
      a crash
    - Fixed a bug where functions which modified reference arguments (such as
      transform, posttransform) would cast the modifiable argument, resulting in
      the result not being set. These functions will now error with invalid
      signatures and require the correct argument types to be provided
    - Fixed an issue which could cause stack overflows when using scalar or
      array (vec/mat) allocations within loops
    - Fixed an issue which could cause a crash when using uninitialized strings
      in compound expressions
    - Fixed a bug where binary string concatenations could cause spurious
      characters to appear with print()
    - Fixed a bug where vec4 types were being interpreted as vec3

    Code Generation:
    - Assignment and binary string operators now always copy the null terminator
    - Added new IR function logic for lerp and for normalization of integer
      vectors
    - The ast::FunctionCall visit now always pushes back the function result
    - Strings are always initialized to the empty string
    - All allocations, aside from string allocs, have been converted to function
      prologue static allocas
    - New codegen/ConstantFolding.h which provides the generic framework for
      constant folding C bindings
    - Renamed the FunctionBase class to FunctionGroup
    - Renamed the FunctionSignatureBase class to Function and re-structured its
      member implementations
    - New classes for derived Functions, CFunctionBase and IRFunctionBase, which
      can be further subclassed for the respective function type.
    - A new interface class SRetFunction for functions which require structural
      returns
    - New framework for building and creating a function. The FunctionBuilder
      provides a way to interface with the function types in FunctionTypes.h
      without having to manually subclass.
    - Exposed the ability to customize llvm function attributes.
    - Removed the unnecessary use of std::function on templated classes and
      methods
    - Re-structured all native functions to use the new FunctionBuilder
    - A variety of IR fixes to some native functions
    - Re-structured all IR functions which required structural returns.
      Previously, these function would produce invalid results unless they were
      forcefully inlined. SRET IR methods now behave in the same way as SRET C
      bindings
    - Improvements to the LLVMType trait structures. These have been mostly
      consolidated to take advantage of new LLVM methods
    - Added a new AliasTypeMap trait structure in Types.h to allow for safer
      and restricted front-end to back-end type mapping
    - New insertStaticAlloca utility method for creating function prologue
      static allocations. New isValidCast utility method to check if a cast is
      valid between two types
    - Removed a number of unused utility methods
    - Removed the function arguments vector from the ComputeGenerator in favor
      of new utility methods to extract arguments (extractArgument)
    - Renamed the ComputeGenerator functions to prefix with ax and suffix
      with the generator type

    Compiler:
    - Initial implementation of LLVM's new PassManager. Currently disabled by
      default. This has not been fully configured to work with target machine
      optimizations.
    - Introduced an internal compiler function, initializeTargetMachine, to
      create the target machine representing the available feature set of the
      host
    - Renamed the mPrioritiseFunctionIR Function Option to mPrioritiseIR and
      defaulted it to ON
    - Introduced a mConstantFoldCBindings Function Option which defaults to ON
    - Added support for AXStringMetadata during global mapping for external
      values

    Math:
    - Exposed the curlnoise functions from StandardFunctions to the
      OpenSimplexNoise headers

    CMake / Build / Testing:
    - New test suites for various back-end utilities
    - Added CMake SIMD options. Note that these optional apply to the compiled
      CXX source files of OpenVDB AX, not the JIT LLVM IR. The IR is always
      optimized to the host specification.
    - Set the required LANGUAGES for OpenVDB AX projects to CXX
    - Added significantly more function tests with profiling options
    - BISON 3.0 is now marked as required if the grammar is to be regenerated
    - New developer options, USE_NEW_PASS_MANAGER and OPENVDB_AX_TEST_PROFILE
    - Travis fixes to build against VDB 6.2.1
    - Removed the deprecated Makefiles

Version 0.1.1 - December 3, 2019

    New Features:
    - Added support for shl and ashr binary and assignment operations
      (<<, >>, <<=, >>=)
    - New functions added: hash, lerp, polardecompose, trace
    - rand() can now be called without a seed argument for improved performance.
      It must still be initialized with a seed at least once for deterministic
      results
    - Exposed the ability to control whether new points attributes or VDB grids
      are automatically created when accessed if they do not exist

    Improvements:
    - Fixed some assertion errors/warnings when compiled with OPENVDB_CXX_STRICT
    - Functions printed from the vdb_ax binary are now ordered alphabetically
    - Simplified the logic in PointExecuterOp when a transform is required
    - Improved the peak memory usage when reading from point positions

    Houdini:
    - New Evaluation Node Path parameter on the OpenVDB AX SOP to allow the SOP
      to be embedded into digital assets which expose the scripting interface
    - Renamed the label from OpenVDB AX to VDB AX and moved the SOP to the VDB
      tab menu

    CMake / Build / Testing:
    - Updated all CMake Find modules to new standards from OpenVDB
    - Improvements to cmake_minimum_required usage
    - Improved the CMake branching for building against a Houdini installation
      when not building the OpenVDB AX SOP
    - Moved codegen/OpenSimplexNoise.h to math/OpenSimplexNoise.h
    - Updated unit test main to match current VDB with additional -shuffle option
    - Replaced the CMake USE_SYSTEM_LIBRARY_PATHS option with
      DISABLE_CMAKE_SEARCH_PATHS and removed the hard coded list of
      SYSTEM_LIBRARY_PATHS in favor of using CMake's default search procedure.
      SYSTEM_LIBRARY_PATHS can still be used as a global list of paths for all
      dependency searches
    - Disabled the python module by default

Version 0.1.0 - August 20, 2019

      Significant API and behaviour changes to OpenVDB AX in this release. Main
      API changes to codegen and AST components. Changes to OpenVDB Volume
      execution involving volumes of differing topologies. Active states now
      only determine the final application of a value rather than their impact on
      other volumes. For example:

        @a = 2;
        @b = @a;

      Irrespective of the overlapping activity, active @b voxels will be set to 2.
      Previously, this would only occur if @a was active at the given location,
      producing an implicit dependency of the active topology of @a on @b.

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
