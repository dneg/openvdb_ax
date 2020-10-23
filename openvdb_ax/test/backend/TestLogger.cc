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

#include "util.h"

#include <openvdb_ax/ast/Parse.h>
#include <openvdb_ax/compiler/Logger.h>
// @note needed for AXLTYPE
#include <openvdb_ax/grammar/generated/axparser.cc>

#include <cppunit/extensions/HelperMacros.h>

namespace {

}
class TestLogger : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestLogger);
    CPPUNIT_TEST(testParseNewNode);
    CPPUNIT_TEST(testParseSetsTree);
    CPPUNIT_TEST(testAddError);
    CPPUNIT_TEST(testAddWarning);
    CPPUNIT_TEST(testWarningsAsErrors);
    CPPUNIT_TEST(testMaxErrors);

    CPPUNIT_TEST_SUITE_END();

    void testParseNewNode();
    void testParseSetsTree();
    void testAddError();
    void testAddWarning();
    void testWarningsAsErrors();
    void testMaxErrors();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestLogger);

void
TestLogger::testParseNewNode()
{
    openvdb::ax::Logger logger;
    axlog = &logger;// setting global Logger* used in parser
    AXLTYPE location;
    location.first_line = 100;
    location.first_column = 65;
    const auto& nodeToLineColMap = logger.mNodeToLineColMap;
    CPPUNIT_ASSERT(nodeToLineColMap.empty());

    const openvdb::ax::ast::Local* testLocal = newNode<openvdb::ax::ast::Local>(&location, "test");

    CPPUNIT_ASSERT_EQUAL(nodeToLineColMap.size(),static_cast<size_t>(1));
    openvdb::ax::Logger::CodeLocation lineCol = nodeToLineColMap.at(testLocal);
    CPPUNIT_ASSERT_EQUAL(lineCol.first, static_cast<size_t>(100));
    CPPUNIT_ASSERT_EQUAL(lineCol.second, static_cast<size_t>(65));
}

void
TestLogger::testParseSetsTree()
{
    openvdb::ax::Logger logger;
    CPPUNIT_ASSERT(!logger.mTreePtr);
    std::string code("");
    openvdb::ax::ast::Tree::ConstPtr tree = openvdb::ax::ast::parse(code.c_str(), logger);
    CPPUNIT_ASSERT(tree);
    CPPUNIT_ASSERT_EQUAL(tree, logger.mTreePtr);
}

void
TestLogger::testAddError()
{
    std::vector<std::string> messages;
    openvdb::ax::Logger logger([&messages](const std::string& message) {
        messages.emplace_back(message);
    });
    CPPUNIT_ASSERT(!logger.hasError());
    CPPUNIT_ASSERT_EQUAL(logger.errors(), messages.size());

    openvdb::ax::Logger::CodeLocation codeLocation(1,1);
    std::string message("test");

    logger.error(message, codeLocation);
    CPPUNIT_ASSERT(logger.hasError());
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] error: test 1:1"), 0);

    logger.error(message, codeLocation);
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(2));
    CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(2));

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasError());
    CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(0));

    openvdb::ax::ast::Local testLocal("name");
    logger.error(message, &testLocal);
    CPPUNIT_ASSERT(logger.hasError());
    CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(3));

    CPPUNIT_ASSERT(!logger.mTreePtr);
    CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] error: test"), 0);

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasError());

    // test that add error finds code location
    {
        openvdb::ax::ast::Tree::ConstPtr tree = openvdb::ax::ast::parse(" a;", logger);
        const Node* local = tree->child(0)->child(0);
        CPPUNIT_ASSERT(local);

        logger.error(message, local);
        CPPUNIT_ASSERT(logger.hasError());
        CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(1));
        CPPUNIT_ASSERT(logger.mTreePtr);
        CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] error: test 1:2"), 0);
    }

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasError());
    // test add error finds code location even when node is deep copy
    {
        openvdb::ax::ast::Tree::ConstPtr tree = openvdb::ax::ast::parse("a;", logger);
        openvdb::ax::ast::Tree::ConstPtr treeCopy(tree->copy());
        const Node* local = tree->child(0)->child(0);
        CPPUNIT_ASSERT(local);
        const Node* localCopy = treeCopy->child(0)->child(0);
        CPPUNIT_ASSERT(localCopy);
        // add referring to copy
        logger.error(message, localCopy);
        CPPUNIT_ASSERT(logger.hasError());
        CPPUNIT_ASSERT_EQUAL(logger.errors(), static_cast<size_t>(1));
        CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(5));

        CPPUNIT_ASSERT(logger.mTreePtr);
        CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] error: test 1:1"), 0);
    }
}

void
TestLogger::testAddWarning()
{
    std::vector<std::string> messages;
    openvdb::ax::Logger logger([](const std::string&) {},
        [&messages](const std::string& message) {
            messages.emplace_back(message);
    });
    CPPUNIT_ASSERT(!logger.hasWarning());
    CPPUNIT_ASSERT_EQUAL(logger.warnings(), messages.size());

    openvdb::ax::Logger::CodeLocation codeLocation(1,1);
    std::string message("test");

    logger.warning(message, codeLocation);
    CPPUNIT_ASSERT(logger.hasWarning());
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] warning: test 1:1"), 0);

    logger.warning(message, codeLocation);
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(2));
    CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(2));

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasWarning());
    CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(0));

    openvdb::ax::ast::Local testLocal("name");
    logger.warning(message, &testLocal);
    CPPUNIT_ASSERT(logger.hasWarning());
    CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(1));
    CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(3));

    CPPUNIT_ASSERT(!logger.mTreePtr);
    CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] warning: test"), 0);

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasWarning());

    // test that add warning finds code location
    {
        openvdb::ax::ast::Tree::ConstPtr tree = openvdb::ax::ast::parse(" a;", logger);
        const Node* local = tree->child(0)->child(0);
        CPPUNIT_ASSERT(local);

        logger.warning(message, local);
        CPPUNIT_ASSERT(logger.hasWarning());
        CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(1));
        CPPUNIT_ASSERT(logger.mTreePtr);
        CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] warning: test 1:2"), 0);
    }

    logger.clear();
    CPPUNIT_ASSERT(!logger.hasWarning());
    // test add warning finds code location even when node is deep copy
    {
        openvdb::ax::ast::Tree::ConstPtr tree = openvdb::ax::ast::parse("a;", logger);
        openvdb::ax::ast::Tree::ConstPtr treeCopy(tree->copy());
        const Node* local = tree->child(0)->child(0);
        CPPUNIT_ASSERT(local);
        const Node* localCopy = treeCopy->child(0)->child(0);
        CPPUNIT_ASSERT(localCopy);
        // add referring to copy
        logger.warning(message, localCopy);
        CPPUNIT_ASSERT(logger.hasWarning());
        CPPUNIT_ASSERT_EQUAL(logger.warnings(), static_cast<size_t>(1));
        CPPUNIT_ASSERT_EQUAL(messages.size(), static_cast<size_t>(5));

        CPPUNIT_ASSERT(logger.mTreePtr);
        CPPUNIT_ASSERT_EQUAL(strcmp(messages.back().c_str(), "[1] warning: test 1:1"), 0);
    }
}

void
TestLogger::testWarningsAsErrors()
{
    openvdb::ax::Logger logger([](const std::string&) {});
    const std::string message("test");
    const openvdb::ax::Logger::CodeLocation location(10,20);
    logger.setWarningsAsErrors(true);
    CPPUNIT_ASSERT(!logger.hasError());
    CPPUNIT_ASSERT(!logger.hasWarning());

    logger.warning(message, location);
    CPPUNIT_ASSERT(logger.hasError());
    CPPUNIT_ASSERT(!logger.hasWarning());
}

void
TestLogger::testMaxErrors()
{
    openvdb::ax::Logger logger([](const std::string&) {});
    const std::string message("test");
    const openvdb::ax::Logger::CodeLocation location(10,20);

    CPPUNIT_ASSERT(logger.error(message, location));
    CPPUNIT_ASSERT(logger.error(message, location));
    CPPUNIT_ASSERT(logger.error(message, location));
    logger.clear();
    logger.setMaxErrors(2);
    CPPUNIT_ASSERT(logger.error(message, location));
    CPPUNIT_ASSERT(!logger.error(message, location));
    CPPUNIT_ASSERT(!logger.error(message, location));
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
