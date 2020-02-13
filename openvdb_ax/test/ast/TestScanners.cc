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

#include <openvdb_ax/ast/AST.h>
#include <openvdb_ax/ast/Scanners.h>
#include <openvdb_ax/test/util.h>

#include <cppunit/extensions/HelperMacros.h>

#include <string>

using namespace openvdb::ax::ast;
using namespace openvdb::ax::ast::tokens;

namespace {

// No dependencies
// - use @a once (read), no other variables
const std::vector<std::string> none = {
    "@a;",
    "@a+1;",
    "@a=1;",
    "@a=func(5);",
    "-@a;",
    "@a[0]=1;",
    "if(true) @a = 1;",
    "if(@a) a=1;",
    "if (@e) if (@b) if (@c) @a;",
    "for (@a; @b; @c) ;",
    "while (@a) ;",
    "for(;true;) @a = 1;"
};

// Self dependencies
// - use @a potentially multiple times (read/write), no other variables
const std::vector<std::string> self = {
    "@a=@a;",
    "@a+=1;",
    "++@a;",
    "@a--;",
    "func(@a);",
    "--@a + 1;",
    "if(@a) @a = 1;",
    "if(@a) ; else @a = 1;",
    "for (@b;@a;@c) @a = 0;",
    "while(@a) @a = 1;"
};

// Code where @a should have a direct dependency on @b only
// - use @a once (read/write), use @b once
const std::vector<std::string> direct = {
    "@a=@b;",
    "@a=-@b;",
    "@a=@b;",
    "@a=1+@b;",
    "@a=func(@b);",
    "@a=++@b;",
    "if(@b) @a=1;",
    "@a=@b.x;",
    "@a=v@b.x;",
    "@a=@b[0];",
    "if(@b) {} else { @a=1; }",
    "for (;@b;) @a = 1;",
    "while (@b) @a = 1;"
};

// Code where @a should have dependencies on @b and c (c always first)
const std::vector<std::string> indirect = {
    "c=@b; @a=c;",
    "c=v@b.x; @a=c;",
    "c=v@b; @a=c[0];",
    "c = {@b,1,2}; @a=c;",
    "int c=@b; @a=c;",
    "int c; c=@b; @a=c;",
    "for(int c = @b; true; e) @a = c;",
    "for(int c = @b;; @a = c) ;",
    "for(int c = @b; c; e) @a = 1;",
    "while(int c = @b) @a = c;",
};
}

class TestScanners : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestScanners);
    CPPUNIT_TEST(testVisitNodeType);
    CPPUNIT_TEST(testFirstLastLocation);
    CPPUNIT_TEST(testVariableDependencies);
    CPPUNIT_TEST_SUITE_END();

    void testVisitNodeType();
    void testFirstLastLocation();
    void testVariableDependencies();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestScanners);

void TestScanners::testVisitNodeType()
{
    size_t count = 0;
    auto counter = [&](const Node&) -> bool {
        ++count; return true;
    };

    // "long@a;"
    Node::Ptr node(new Attribute("a", CoreType::LONG));

    visitNodeType<Node>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    count = 0;
    visitNodeType<Local>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(0), count);

    count = 0;
    visitNodeType<Variable>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    count = 0;
    visitNodeType<Attribute>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    // "{1.0f, 2.0, 3};"
    node.reset(new ArrayPack(
        new ExpressionList({
            new Value<float>(1.0f),
            new Value<double>(2.0),
            new Value<int64_t>(3)
        })));

    count = 0;
    visitNodeType<Node>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(5), count);

    count = 0;
    visitNodeType<Local>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(0), count);

    count = 0;
    visitNodeType<ValueBase>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(3), count);

    count = 0;
    visitNodeType<ArrayPack>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    count = 0;
    visitNodeType<ExpressionList>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    count = 0;
    visitNodeType<Expression>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(5), count);

    count = 0;
    visitNodeType<Statement>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(5), count);

    // "@a += v@b.x = x %= 1;"
    // @note 13 explicit nodes + an implicit ExpressionList
    // which is created by the ArrayUnpack and stores the components
    node.reset(new AssignExpression(
        new Attribute("a", CoreType::FLOAT, true),
        new BinaryOperator(
            OperatorToken::PLUS,
            new Attribute("a", CoreType::FLOAT, true),
            new AssignExpression(
                new ArrayUnpack(
                    new Attribute("b", CoreType::VEC3F),
                    new Value<int32_t>(0)
                ),
                new AssignExpression(
                    new Local("x"),
                    new BinaryOperator(
                        OperatorToken::MODULO,
                        new Local("x"),
                        new Value<int32_t>(1)
                    ),
                    true
                ),
                false
            )
        ),
        true
    ));

    count = 0;
    visitNodeType<Node>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(14), count);

    count = 0;
    visitNodeType<Local>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(2), count);

    count = 0;
    visitNodeType<Value<int>>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(2), count);

    count = 0;
    visitNodeType<ArrayPack>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(0), count);

    count = 0;
    visitNodeType<ArrayUnpack>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);

    count = 0;
    visitNodeType<AssignExpression>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(3), count);

    count = 0;
    visitNodeType<Expression>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(14), count);

    count = 0;
    visitNodeType<ExpressionList>(*node, counter);
    CPPUNIT_ASSERT_EQUAL(size_t(1), count);
}

void TestScanners::testFirstLastLocation()
{
    // The list of above code sets which are expected to have the same
    // first and last use of @a.
    const std::vector<const std::vector<std::string>*> snippets {
        &none,
        &direct,
        &indirect
    };

    for (const auto& samples : snippets) {
        for (const std::string& code : *samples) {
            const Tree::Ptr tree = parse(code.c_str());
            CPPUNIT_ASSERT(tree);
            const Variable* first = firstUse(*tree, "@a");
            const Variable* last = lastUse(*tree, "@a");
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Unable to locate first @a AST node", code), first);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Unable to locate last @a AST node", code), last);
            CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid first/last AST node comparison", code),
                first == last);
        }
    }
}

void TestScanners::testVariableDependencies()
{
    for (const std::string& code : none) {
        const Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT(tree);
        const Variable* last = lastUse(*tree, "@a");
        CPPUNIT_ASSERT(last);

        std::vector<const Variable*> vars;
        variableDependencies(*last, vars);
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Expected 0 deps", code),
            vars.empty());
    }

    for (const std::string& code : self) {
        const Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT(tree);
        const Variable* last = lastUse(*tree, "@a");
        CPPUNIT_ASSERT(last);

        std::vector<const Variable*> vars;
        variableDependencies(*last, vars);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dep size", code),
            1ul, vars.size());

        const Variable* var = vars.front();
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            var->isType<Attribute>());
        const Attribute* attrib = static_cast<const Attribute*>(var);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            std::string("float@a"), attrib->tokenname());
    }

    for (const std::string& code : direct) {
        const Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT(tree);
        const Variable* last = lastUse(*tree, "@a");
        CPPUNIT_ASSERT(last);

        std::vector<const Variable*> vars;
        variableDependencies(*last, vars);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dep size", code),
            1ul, vars.size());

        const Variable* var = vars.front();
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            var->isType<Attribute>());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            std::string("b"), var->name());
    }

    for (const std::string& code : indirect) {
        const Tree::Ptr tree = parse(code.c_str());
        CPPUNIT_ASSERT(tree);
        const Variable* last = lastUse(*tree, "@a");
        CPPUNIT_ASSERT(last);

        std::vector<const Variable*> vars;
        variableDependencies(*last, vars);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dep size", code),
            2ul, vars.size());

        // check c
        const Variable* var = vars[0];
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            var->isType<Local>());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            std::string("c"), var->name());

        // check @b
        var = vars[1];
        CPPUNIT_ASSERT_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            var->isType<Attribute>());
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MSG("Invalid variable dependency", code),
            std::string("b"), var->name());
    }

    // Test a more complicated code snippet. Note that this also checks the
    // order which isn't strictly necessary

    const std::string complex =
        "int a = func(1,@e);"
        "pow(@d, a);"
        "mat3f m = 0;"
        "scale(m, v@v);"
        ""
        "float f1 = 0;"
        "float f2 = 0;"
        "float f3 = 0;"
        ""
        "f3 = @f;"
        "f2 = f3;"
        "f1 = f2;"
        "if (@a - @e > f1) {"
        "    @b = func(m);"
        "    if (true) {"
        "        ++@c[0] = a;"
        "    }"
        "}";

    const Tree::Ptr tree = parse(complex.c_str());
    CPPUNIT_ASSERT(tree);
    const Variable* lasta = lastUse(*tree, "@a");
    const Variable* lastb = lastUse(*tree, "@b");
    const Variable* lastc = lastUse(*tree, "@c");
    const Variable* lastd = lastUse(*tree, "@d");
    const Variable* laste = lastUse(*tree, "@e");
    const Variable* lastf = lastUse(*tree, "@f");
    const Variable* lastv = lastUse(*tree, "vec3f@v");
    CPPUNIT_ASSERT(lasta);
    CPPUNIT_ASSERT(lastb);
    CPPUNIT_ASSERT(lastc);
    CPPUNIT_ASSERT(lastd);
    CPPUNIT_ASSERT(laste);
    CPPUNIT_ASSERT(lastf);
    CPPUNIT_ASSERT(lastv);

    std::vector<const Variable*> vars;
    variableDependencies(*lasta, vars);
    CPPUNIT_ASSERT(vars.empty());

    // @b should depend on: m, v@v, m, @a, @e, @e, f1, f2, f3, @f
    variableDependencies(*lastb, vars);
    CPPUNIT_ASSERT_EQUAL(10ul, vars.size());
    CPPUNIT_ASSERT(vars[0]->isType<Local>());
    CPPUNIT_ASSERT(vars[0]->name() == "m");
    CPPUNIT_ASSERT(vars[1]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[1])->tokenname() == "vec3f@v");
    CPPUNIT_ASSERT(vars[2]->isType<Local>());
    CPPUNIT_ASSERT(vars[2]->name() == "m");
    CPPUNIT_ASSERT(vars[3]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[3])->tokenname() == "float@a");
    CPPUNIT_ASSERT(vars[4]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[4])->tokenname() == "float@e");
    CPPUNIT_ASSERT(vars[5]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[5])->tokenname() == "float@e");
    CPPUNIT_ASSERT(vars[6]->isType<Local>());
    CPPUNIT_ASSERT(vars[6]->name() == "f1");
    CPPUNIT_ASSERT(vars[7]->isType<Local>());
    CPPUNIT_ASSERT(vars[7]->name() == "f2");
    CPPUNIT_ASSERT(vars[8]->isType<Local>());
    CPPUNIT_ASSERT(vars[8]->name() == "f3");
    CPPUNIT_ASSERT(vars[9]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[9])->tokenname() == "float@f");

    // @c should depend on: @c, a, @d, a, @e, @a, @e, f1, f2, f3, @f
    vars.clear();
    variableDependencies(*lastc, vars);
    CPPUNIT_ASSERT_EQUAL(11ul, vars.size());
    CPPUNIT_ASSERT(vars[0]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[0])->tokenname() == "float@c");
    CPPUNIT_ASSERT(vars[1]->isType<Local>());
    CPPUNIT_ASSERT(vars[1]->name() == "a");
    CPPUNIT_ASSERT(vars[2]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[2])->tokenname() == "float@d");
    CPPUNIT_ASSERT(vars[3]->isType<Local>());
    CPPUNIT_ASSERT(vars[3]->name() == "a");
    CPPUNIT_ASSERT(vars[4]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[4])->tokenname() == "float@e");
    CPPUNIT_ASSERT(vars[5]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[5])->tokenname() == "float@a");
    CPPUNIT_ASSERT(vars[6]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[6])->tokenname() == "float@e");
    CPPUNIT_ASSERT(vars[7]->isType<Local>());
    CPPUNIT_ASSERT(vars[7]->name() == "f1");
    CPPUNIT_ASSERT(vars[8]->isType<Local>());
    CPPUNIT_ASSERT(vars[8]->name() == "f2");
    CPPUNIT_ASSERT(vars[9]->isType<Local>());
    CPPUNIT_ASSERT(vars[9]->name() == "f3");
    CPPUNIT_ASSERT(vars[10]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[10])->tokenname() == "float@f");

    // @d should depend on: @d, a, @e
    vars.clear();
    variableDependencies(*lastd, vars);
    CPPUNIT_ASSERT_EQUAL(3ul, vars.size());
    CPPUNIT_ASSERT(vars[0]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[0])->tokenname() == "float@d");
    CPPUNIT_ASSERT(vars[1]->isType<Local>());
    CPPUNIT_ASSERT(vars[1]->name() == "a");
    CPPUNIT_ASSERT(vars[2]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[2])->tokenname() == "float@e");

    // @e should depend on itself
    vars.clear();
    variableDependencies(*laste, vars);
    CPPUNIT_ASSERT_EQUAL(1ul, vars.size());
    CPPUNIT_ASSERT(vars[0]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[0])->tokenname() == "float@e");

    // @f should depend on nothing
    vars.clear();
    variableDependencies(*lastf, vars);
    CPPUNIT_ASSERT(vars.empty());

    // @v should depend on: m, v@v
    vars.clear();
    variableDependencies(*lastv, vars);
    CPPUNIT_ASSERT_EQUAL(2ul, vars.size());
    CPPUNIT_ASSERT(vars[0]->isType<Attribute>());
    CPPUNIT_ASSERT(static_cast<const Attribute*>(vars[0])->tokenname() == "vec3f@v");
    CPPUNIT_ASSERT(vars[1]->isType<Local>());
    CPPUNIT_ASSERT(vars[1]->name() == "m");
}

// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
