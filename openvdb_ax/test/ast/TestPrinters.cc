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

#include "../ast/AST.h"
#include "../ast/Parse.h"
#include "../ast/PrintTree.h"

#include <cppunit/extensions/HelperMacros.h>

#include <string>
#include <ostream>

using namespace openvdb::ax::ast;
using namespace openvdb::ax::ast::tokens;

class TestPrinters : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestPrinters);
    CPPUNIT_TEST(testReprint);
    CPPUNIT_TEST_SUITE_END();

    void testReprint();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestPrinters);

void TestPrinters::testReprint()
{
    // Small function providing more verbose output on failures
    auto check = [](const std::string& in, const std::string& expected) {
        const size_t min = std::min(in.size(), expected.size());
        for (size_t i = 0; i < min; ++i) {
            if (in[i] != expected[i]) {
                std::ostringstream msg;
                msg << "TestReprint failed at character " << i << '.'
                    << '[' << in[i] << "] vs [" << expected[i] << "]\n"
                    << "Got:\n" << in
                    << "Expected:\n" << expected;
                CPPUNIT_FAIL(msg.str());
            }
        }
        if (in.size() != expected.size()) {
            std::ostringstream msg;
            msg << "TestReprint failed at end character.\n"
                << "Got:\n" << in
                << "Expected:\n" << expected ;
            CPPUNIT_FAIL(msg.str());
        }
    };

    std::ostringstream os;

    // Test binary ops
    std::string in = "a + b * c / d % e << f >> g = h & i | j ^ k && l || m;";
    std::string expected = "(((a + (((b * c) / d) % e)) << f) >> g = ((((h & i) | (j ^ k)) && l) || m));\n";
    Tree::ConstPtr tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test binary ops paren
    os.str("");
    in = "(a + b) * (c / d) % e << (f) >> g = (((h & i) | j) ^ k) && l || m;";
    expected = "(((((a + b) * (c / d)) % e) << f) >> g = (((((h & i) | j) ^ k) && l) || m));\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test relational
    os.str("");
    in = "a <= b; c >= d; e == f; g != h; i < j; k > l;";
    expected = "(a <= b);\n(c >= d);\n(e == f);\n(g != h);\n(i < j);\n(k > l);\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test assignments
    os.str("");
    in = "a = b; b += c; c -= d; d /= e; e *= f; f %= 1; g &= 2; h |= 3; i ^= 4; j <<= 5; k >>= 6;";
    expected = "a = b;\nb += c;\nc -= d;\nd /= e;\ne *= f;\nf %= 1;\ng &= 2;\nh |= 3;\ni ^= 4;\nj <<= 5;\nk >>= 6;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test crement
    os.str("");
    in = "++++a; ----b; a++; b--;";
    expected = "++++a;\n----b;\na++;\nb--;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test comma
    os.str("");
    in = "a,b,(c,d),(e,(f,(g,h,i)));";
    expected = "(a, b, (c, d), (e, (f, (g, h, i))));\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test array unpack
    os.str("");
    in = "a.x; b.y; c.z; d[0]; d[1,2]; e[(a.r, c.b), b.g];";
    expected = "a[0];\nb[1];\nc[2];\nd[0];\nd[1, 2];\ne[(a[0], c[2]), b[1]];\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test array pack
    os.str("");
    in = "a = {0,1}; b = {2,3,4}; c = {a,(b,c), d};";
    expected = "a = {0, 1};\nb = {2, 3, 4};\nc = {a, (b, c), d};\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test declarations
    os.str("");
    in = "bool a; short b,c; int d=0, e; long f; float g; double h, i=0;";
    expected = "bool a;\nshort b, c;\nint d = 0, e;\nlong f;\nfloat g;\ndouble h, i = 0;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test conditionals
    os.str("");
    in = "if (a) b; else if (c) d; else e; if (a) if (b) { c,d; } else { e,f; }";
    expected = "if (a)\n{\nb;\n}\nelse\n{\nif (c)\n{\nd;\n}\nelse\n{\ne;\n}\n}\nif (a)\n{\nif (b)\n{\n(c, d);\n}\nelse\n{\n(e, f);\n}\n}\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test keywords
    os.str("");
    in = "return; break; continue; true; false;";
    expected = "return;\nbreak;\ncontinue;\ntrue;\nfalse;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test attributes/externals
    os.str("");
    in = "@a; $a; v@b; v$b; f@a; f$a; i@c; i$c; s@d; s$d;";
    expected = "float@a;\nfloat$a;\nvec3f@b;\nvec3f$b;\nfloat@a;\nfloat$a;\nint@c;\nint$c;\nstring@d;\nstring$d;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test ternary
    os.str("");
    in = "a ? b : c; a ? b ? c ? : d : e : f;";
    expected = "a ? b : c;\na ? b ? c ?: d : e : f;\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test loops
    os.str("");
    in = "while (a) for (int b, c;;) do { d; } while (e)";
    expected = "while (a)\n{\nfor (int b, c; true; )\n{\ndo\n{\nd;\n}\nwhile (e)\n}\n}\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "");
    check(os.str(), ("{\n" + expected + "}\n"));

    // Test loops with indents
    os.str("");
    in = "while (a) for (int b, c;;) do { d; } while (e)";
    expected = "  while (a)\n  {\n    for (int b, c; true; )\n    {\n      do\n      {\n        d;\n      }\n      while (e)\n    }\n  }\n";
    tree = parse(in.c_str());
    CPPUNIT_ASSERT(tree.get());
    reprint(*tree, os, "  ");
    check(os.str(), ("{\n" + expected + "}\n"));
}


// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
