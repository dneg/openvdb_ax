///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2020 DNEG
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DNEG nor the names
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

#include "Parse.h"
#include "../Exceptions.h"

// if OPENVDB_AX_REGENERATE_GRAMMAR is defined, we've re-generated the
// grammar - include path should be set up to pull in from the temp dir
// @note We need to include this to get access to axlloc. Should look to
//   re-work this so we don't have to (would require a reentrant parser)
#ifdef OPENVDB_AX_REGENERATE_GRAMMAR
#include "axparser.h"
#else
#include "../grammar/generated/axparser.h"
#endif

#include <tbb/mutex.h>
#include <string>
#include <memory>

namespace {
// Declare this at file scope to ensure thread-safe initialization.
tbb::mutex sInitMutex;
}

openvdb::ax::Logger* axlog = nullptr;
using YY_BUFFER_STATE = struct yy_buffer_state*;
extern int axparse(openvdb::ax::ast::Tree**);
extern YY_BUFFER_STATE ax_scan_string(const char * str);
extern void ax_delete_buffer(YY_BUFFER_STATE buffer);
extern void axerror (openvdb::ax::ast::Tree**, char const *s) {
    //@todo: add check for memory exhaustion
    assert(axlog);
    axlog->error(/*starts with 'syntax error, '*/s + 14,
        {axlloc.first_line, axlloc.first_column});
}

openvdb::ax::ast::Tree::ConstPtr
openvdb::ax::ast::parse(const char* code, openvdb::ax::Logger& logger)
{
    tbb::mutex::scoped_lock lock(sInitMutex);
    axlog = &logger; // for lexer errs
    logger.setSourceCode(code);

    // reset all locations
    axlloc.first_line = axlloc.last_line = 1;
    axlloc.first_column = axlloc.last_column = 1;

    YY_BUFFER_STATE buffer = ax_scan_string(code);

    openvdb::ax::ast::Tree* tree(nullptr);
    axparse(&tree);
    axlog = nullptr;

    openvdb::ax::ast::Tree::ConstPtr ptr(const_cast<const openvdb::ax::ast::Tree*>(tree));

    ax_delete_buffer(buffer);

    logger.setSourceTree(ptr);
    return ptr;
}


openvdb::ax::ast::Tree::Ptr
openvdb::ax::ast::parse(const char* code)
{
    openvdb::ax::Logger logger(
        [](const std::string& error) {
            OPENVDB_THROW(openvdb::AXSyntaxError, error);
        });

    openvdb::ax::ast::Tree::ConstPtr constTree = openvdb::ax::ast::parse(code, logger);

    return std::const_pointer_cast<openvdb::ax::ast::Tree>(constTree);
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
