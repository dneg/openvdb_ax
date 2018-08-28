///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2018 DNEG Visual Effects
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG Visual Effects nor the names
// of its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/Scanners.h>
#include <openvdb_ax/codegen/FunctionRegistry.h>
#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/compiler/PointExecutable.h>
#include <openvdb_ax/compiler/VolumeExecutable.h>

#include <openvdb/openvdb.h>
#include <openvdb/util/logging.h>
#include <openvdb/points/PointDelete.h>
#include <openvdb/pointsdev/PointSort.h>

#ifdef DWA_OPENVDB
#include <usagetrack.h>
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const char* gProgName = "";

struct ProgOptions
{
    std::string mInputCode = "";
    std::string mInputVDBFile = "";
    std::string mOutputVDBFile = "";
    bool mVerbose = false;
};

void
usage [[noreturn]] (int exitStatus = EXIT_FAILURE)
{
    std::cerr <<
"Usage: " << gProgName << " input.vdb output.vdb [ -s \"string\" | -f file.txt ] [OPTIONS]\n" <<
"Which: executes a string or file containing a code snippet on an input.vdb file\n\n" <<
"Options:\n" <<
"    -s snippet        execute code snippet on the input.vdb file\n" <<
"    -f file.txt       execute text file containing a code snippet on the input.vdb file\n" <<
"    -v                verbose (print timing and diagnostics)\n" <<
"    --list-functions  list all available functions, their signatures and their documentation\n" <<
"Warning:\n" <<
"     Providing the same file-path to both input.vdb and output.vdb arguments will overwrite\n" <<
"     the file. If no output file is provided, the input.vdb will be processed but will remain\n" <<
"     unchanged on disk (this is useful for testing the success status of code).\n";
    exit(exitStatus);
}

void loadSnippetFile(const std::string& fileName, std::string& textString)
{
    std::ifstream in(fileName.c_str(), std::ios::in | std::ios::binary);

    if (!in) {
        OPENVDB_LOG_FATAL("File Load Error: " << fileName);
        usage();
    }

    textString =
        std::string(std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>());
}

struct OptParse
{
    int argc;
    char** argv;

    OptParse(int argc_, char* argv_[]): argc(argc_), argv(argv_) {}

    bool check(int idx, const std::string& name, int numArgs = 1) const
    {
        if (argv[idx] == name) {
            if (idx + numArgs >= argc) {
                OPENVDB_LOG_FATAL("option " << name << " requires "
                    << numArgs << " argument" << (numArgs == 1 ? "" : "s"));
                usage();
            }
            return true;
        }
        return false;
    }
};

struct ScopedInitialize
{
    ScopedInitialize(int argc, char *argv[]) {
        openvdb::logging::initialize(argc, argv);
        openvdb::initialize();
    }

    ~ScopedInitialize() {
        if (openvdb::ax::isInitialized()) {
            openvdb::ax::uninitialize();
        }
        openvdb::uninitialize();
    }

    inline void initializeCompiler() const { openvdb::ax::initialize(); }
};

void printFunctions(std::ostream& os)
{
    openvdb::ax::FunctionOptions opts;
    opts.mLazyFunctions = false;

    static const size_t maxHelpTextWidth = 100;

    const openvdb::ax::codegen::FunctionRegistry::UniquePtr reg =
        openvdb::ax::codegen::createStandardRegistry(opts);

    llvm::LLVMContext C;

    for (const auto& iter : reg->map()) {

        if (iter.second.isInternal()) continue;

        os << iter.first << std::endl << "|" << std::endl;

        const openvdb::ax::codegen::FunctionBase::Ptr function = iter.second.function();
        std::string docs;
        function->getDocumentation(docs);

        if (docs.empty()) docs = "<No documentation exists for this fuction>";

        // do some basic formatting on the help text

        size_t pos = maxHelpTextWidth;
        while (pos < docs.size()) {
            while (docs[pos] != ' ' && pos != 0) --pos;
            if (pos == 0) break;
            docs.insert(pos, "\n|  ");
            pos += maxHelpTextWidth;
        }

        os << "| - " << docs << std::endl << "|" << std::endl;

        const auto& list = function->list();
        for (const openvdb::ax::codegen::FunctionSignatureBase::Ptr& signature : list) {
            os << "|  - ";
            signature->print(C, iter.first, os);
            os << std::endl;
        }
        os << std::endl;
    }
}

int
main(int argc, char *argv[])
{
    OPENVDB_START_THREADSAFE_STATIC_WRITE
    gProgName = argv[0];
    const char* ptr = ::strrchr(gProgName, '/');
    if (ptr != nullptr) gProgName = ptr + 1;
    OPENVDB_FINISH_THREADSAFE_STATIC_WRITE

    if (argc == 1) usage();

    ScopedInitialize initializer(argc, argv);
    OptParse parser(argc, argv);
    ProgOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-') {
            if (parser.check(i, "-s")) {
                ++i;
                options.mInputCode = argv[i];
            } else if (parser.check(i, "-f")) {
                ++i;
                loadSnippetFile(argv[i], options.mInputCode);
            } else if (parser.check(i, "-v", 0)) {
                options.mVerbose = true;
            } else if (parser.check(i, "--list-functions", 0)) {
                initializer.initializeCompiler();
                printFunctions(std::cout);
                return EXIT_SUCCESS;
            } else if (arg == "-h" || arg == "-help" || arg == "--help") {
                usage(EXIT_SUCCESS);
            } else {
                OPENVDB_LOG_FATAL("\"" + arg + "\" is not a valid option");
                usage();
            }
        } else if (!arg.empty()) {
            if (options.mInputVDBFile.empty()) {
                options.mInputVDBFile = arg;
            }
            else if (options.mOutputVDBFile.empty()) {
                options.mOutputVDBFile = arg;
            }
            else {
                OPENVDB_LOG_FATAL("unrecognized positional argument \"" << arg << "\"");
                usage();
            }
        } else {
            usage();
        }
    }

    if (options.mInputVDBFile.empty() || options.mInputCode.empty()) {
        OPENVDB_LOG_FATAL("expected at least one OpenVDB file and one code snippet");
        usage();
    }

    if (options.mOutputVDBFile.empty()) {
        OPENVDB_LOG_WARN("no output VDB File specified - nothing will be written to disk");
    }


    openvdb::GridPtrVecPtr grids;
    openvdb::MetaMap::Ptr meta;
    openvdb::io::File file(options.mInputVDBFile);

    try {
        file.open();
        grids = file.getGrids();
        meta = file.getMetadata();
        file.close();
    } catch (openvdb::Exception& e) {
        OPENVDB_LOG_ERROR(e.what() << " (" << options.mInputVDBFile << ")");
        return EXIT_FAILURE;
    }

    assert(meta);
    assert(grids);

    // begin compiler

    initializer.initializeCompiler();
    openvdb::ax::Compiler::Ptr compiler = openvdb::ax::Compiler::create();

    // Execute on PointDataGrids

    bool executeOnPoints = false;
    for (auto grid : *grids) {
        if (grid->isType<openvdb::points::PointDataGrid>()) {
            executeOnPoints = true;
            break;
        }
    }

    if (executeOnPoints) {

        using openvdb::ax::PointExecutable;

        openvdb::ax::CustomData::Ptr customData = openvdb::ax::CustomData::create();
        PointExecutable::Ptr pointExecutable;

        const openvdb::ax::ast::Tree::ConstPtr syntaxTree =
            openvdb::ax::ast::parse(options.mInputCode.c_str());

        if (options.mVerbose) std::cout << "OpenVDB PointDataGrids Found" << std::endl;
        std::vector<std::string> warnings;

        try {
            if (options.mVerbose) std::cout << "  Compiling for PointDataGrids...";
            pointExecutable = compiler->compile<PointExecutable>(*syntaxTree, customData, &warnings);
        } catch (std::exception& e) {
            OPENVDB_LOG_FATAL("Compilation error!");
            OPENVDB_LOG_FATAL("Errors:");
            OPENVDB_LOG_FATAL(e.what());
            return EXIT_FAILURE;
        }

        for (const std::string& warning : warnings) {
            OPENVDB_LOG_WARN(warning);
        }

        if (options.mVerbose) std::cout << "done." << std::endl;

        for (auto grid : *grids) {
            if (!grid->isType<openvdb::points::PointDataGrid>()) continue;

            openvdb::points::PointDataGrid::Ptr points =
               openvdb::gridPtrCast<openvdb::points::PointDataGrid>(grid);
            if (options.mVerbose) std::cout << "  Executing on \"" + points->getName() + "\"...";

            try {
                pointExecutable->execute(*points);

                const bool requiresDeletion =
                    openvdb::ax::ast::callsFunction(*syntaxTree, "deletepoint");

                if (requiresDeletion) {
                    openvdb::points::deleteFromGroup(points->tree(), "dead", false, false);
                }
            }
            catch (std::exception& e) {
                OPENVDB_LOG_FATAL("Execution error!");
                OPENVDB_LOG_FATAL("Errors:");
                OPENVDB_LOG_FATAL(e.what());
                return EXIT_FAILURE;
            }

            if (options.mVerbose) std::cout << "done." << std::endl << std::endl;
        }
    }

    // Execute on Volumes

    bool executeOnVolumes = false;
    for (auto grid : *grids) {
        if (!grid->isType<openvdb::points::PointDataGrid>()) {
            executeOnVolumes = true;
            break;
        }
    }

    if (executeOnVolumes) {
        openvdb::ax::CustomData::Ptr customData = openvdb::ax::CustomData::create();

        using openvdb::ax::VolumeExecutable;
        VolumeExecutable::Ptr volumeExecutable;

        if (options.mVerbose) std::cout << "OpenVDB Volume Grids Found" << std::endl;
        std::vector<std::string> warnings;

        try {
            if (options.mVerbose) std::cout << "  Compiling for Volume VDB Grid...";
            volumeExecutable =
                compiler->compile<VolumeExecutable>(options.mInputCode, customData, &warnings);
        } catch (std::exception& e) {
            OPENVDB_LOG_FATAL("Compilation error!");
            OPENVDB_LOG_FATAL("Errors:");
            OPENVDB_LOG_FATAL(e.what());
            return EXIT_FAILURE;
        }

        for (const std::string& warning : warnings) {
            OPENVDB_LOG_WARN(warning);
        }

        if (options.mVerbose) std::cout << "done." << std::endl;

        if (options.mVerbose) {
            std::string names("");
            for (auto grid : *grids) {
                if (grid->isType<openvdb::points::PointDataGrid>()) continue;
                names += grid->getName() + ", ";
            }

            names.pop_back();
            names.pop_back();
            std::cout << "  Executing using \"" + names + "\"...";
        }

        try {
            volumeExecutable->execute(*grids);
        } catch (std::exception& e) {
            OPENVDB_LOG_FATAL("Execution error!");
            OPENVDB_LOG_FATAL("Errors:");
            OPENVDB_LOG_FATAL(e.what());
            return EXIT_FAILURE;
        }

        if (options.mVerbose) std::cout << "done." << std::endl;
    }

    if (!options.mOutputVDBFile.empty()) {
        openvdb::io::File out(options.mOutputVDBFile);

        try {
            out.write(*grids, *meta);
        } catch (openvdb::Exception& e) {
            OPENVDB_LOG_ERROR(e.what() << " (" << out.filename() << ")");
        }
    }

    return EXIT_SUCCESS;
}

// Copyright (c) 2015-2018 DNEG Visual Effects
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
