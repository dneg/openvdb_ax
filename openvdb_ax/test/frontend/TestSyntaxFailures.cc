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

#include <cppunit/extensions/HelperMacros.h>

#include <openvdb_ax/compiler/Compiler.h>
#include <openvdb_ax/Exceptions.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "test/util.h"

namespace {

// mimics std::pair<std::string, null>
struct StrWrapper {
    StrWrapper(const char* str) : first(str) {}
    const std::string first;
};

static const std::vector<StrWrapper> tests {
    // invalid r-value syntax
    "@a = @;",
    "@a = =;",
    "@a = +;",
    "@a = -;",
    "@a = *;",
    "@a = /;",
    "@a = %;",
    "@a = |;",
    "@a = &;",
    "@a = ^;",
    "@a = ~;",
    "@a = ==;",
    "@a = !=;",
    "@a = >;",
    "@a = <;",
    "@a = >=;",
    "@a = <=;",
    "@a = +=;",
    "@a = -=;",
    "@a = *=;",
    "@a = /=;",
    "@a = ++;",
    "@a = --;",
    "@a = &&;",
    "@a = ||;",
    "@a = !;",
    "@a = ,;",
    "@a = (;",
    "@a = );",
    "@a = {;",
    "@a =};",
    "@a = .x;",
    "@a = .y;",
    "@a = .z;",
    "@a = .r;",
    "@a = .g;",
    "@a = .b;",
    "@a = f@;",
    "@a = i@;",
    "@a = v@;",
    "@a = s@;",
    "@a = if;",
    "@a = else;",
    "@a = return;",
    "@a = ;",
    "@a = {};",
    "@a = \"a;",
    "v@a.a = 0;",

    "$a = $;",
    "$a = =;",
    "$a = +;",
    "$a = -;",
    "$a = *;",
    "$a = /;",
    "$a = %;",
    "$a = |;",
    "$a = &;",
    "$a = ^;",
    "$a = ~;",
    "$a = ==;",
    "$a = !=;",
    "$a = >;",
    "$a = <;",
    "$a = >=;",
    "$a = <=;",
    "$a = +=;",
    "$a = -=;",
    "$a = *=;",
    "$a = /=;",
    "$a = ++;",
    "$a = --;",
    "$a = &&;",
    "$a = ||;",
    "$a = !;",
    "$a = ,;",
    "$a = (;",
    "$a = );",
    "$a = {;",
    "$a =};",
    "$a = .x;",
    "$a = .y;",
    "$a = .z;",
    "$a = .r;",
    "$a = .g;",
    "$a = .b;",
    "$a = f$;",
    "$a = i$;",
    "$a = v$;",
    "$a = s$;",
    "$a = if;",
    "$a = else;",
    "$a = return;",
    "$a = ;",
    "$a = {};",
    "$a = {1};",
    "$a = \"a;",
    "v$a[0] = 0;",
    "v$a.a = 0;",

    // @todo these should probably be valid syntax and the code
    // generators should handle assignments based on the current
    // r/lvalues
    "5 = 5;",
    "$a = 5;",

    // invalid l-value
    // TODO: these should fail syntax tests
    // {"+@a = 0;", },
    // {"-@a = 0;", },
    // {"~@a = 0;", },
    // {"!@a = 0;", },
    // "++@a = 0;",
    // "--@a = 0;",
    "=@a;",
    "*@a;",
    "/@a;",
    "%@a;",
    "|@a;",
    "&@a;",
    "^@a;",
    "==@a;",
    "!=@a;",
    ">@a;",
    "<@a;",
    ">=@a;",
    "<=@a;",
    "+=@a;",
    "-=@a;",
    "*=@a;",
    "/=@a;",
    "&&@a;",
    "||@a;",
    ",@a;",
    "(@a;",
    ")@a;",
    "{@a;",
    "}@a;",
    ".x@a;",
    ".y@a;",
    ".z@a;",
    ".r@a;",
    ".g@a;",
    ".b@a;",
    "@@a;",
    "f@@a;",
    "i@@a;",
    "v@@a;",
    "s@@a;",
    "if@a;",
    "else@a;",
    "return@a;",
    "{1}@a;",
    "\"a\"@a;",
    "b@a;",
    "sht@a;",
    "it@a;",
    "l@a;",
    "flt@a;",
    "dbl@a;",
    "vecint@a;",
    "vint@a;",
    "vfloat@a;",
    "vecflt@a;",
    "vflt@a;",
    "vdouble@a;",
    "vecdbl@a;",
    "vdbl@a;",
    "str@a;",

    "++$a = 0;",
    "--$a = 0;",
    "=$a;",
    "*$a;",
    "/$a;",
    "%$a;",
    "|$a;",
    "&$a;",
    "^$a;",
    "==$a;",
    "!=$a;",
    ">$a;",
    "<$a;",
    ">=$a;",
    "<=$a;",
    "+=$a;",
    "-=$a;",
    "*=$a;",
    "/=$a;",
    "&&$a;",
    "||$a;",
    ",$a;",
    "($a;",
    ")$a;",
    "{$a;",
    "}$a;",
    ".x$a;",
    ".y$a;",
    ".z$a;",
    ".r$a;",
    ".g$a;",
    ".b$a;",
    "$$a;",
    "f$$a;",
    "i$$a;",
    "v$$a;",
    "s$$a;",
    "if$a;",
    "else$a;",
    "return$a;",
    "{1}$a;",
    "\"a\"$a;",
    "b$a;",
    "sht$a;",
    "it$a;",
    "l$a;",
    "flt$a;",
    "dbl$a;",
    "vecint$a;",
    "vint$a;",
    "vfloat$a;",
    "vecflt$a;",
    "vflt$a;",
    "vdouble$a;",
    "vecdbl$a;",
    "vdbl$a;",
    "str$a;",

    "a ! a;",
    "a ~ a;",
    "a \\ a;",
    "a ? a;",
    "bool + a;",
    "bool a + a;",
    "return + a;",
    "if + a;",
    "a + if(true) {};",
    "{} + {};",
    "~ + !;",
    "+ + -;",
    "; + ;",
    "int();",
    "int(return);",
    "int(if(true) {});",
    "int(;);",
    "int(bool a;);",
    "int(bool a);",
    "int{a};",
    "int[a];",
    "string(a);",
    "vector(a);",
    "vec3i(a);",
    "vec3f(a);",
    "vec3d(a);",

    // invalid if block
    "if (a) {b}",
    "if (a) else ();",
    "if (); else (a);",
    "if (a) if(b) {if (c)} else {}",
    "if (if(a));",
    "if ();",
    "if (); else ;",
    "if (); else ();",
    "if (); else () {}",
    "if (); elf {}",
    "if (a) {} elif (b) {}",
    "else {}",
    "else ;",
    "if a;",
    "if a {} elif b {}",
    "if (a); else ; else ;",
    "else (a); ",
    "if (a) {}; else {};",
    "if (a) {b = 1} else {};",
    "if (a) {} ; else {}",
    "if () {}; else (a);",

    // invalid ternary
   "?;",
   ":;",
   "? :;",
   "? : false;",
   "true ? :;",
   "true ? false;",
   "true ? false :;",
   "true : 1 ? 2;",
   "true ? 1 ? 2;",
   "true : 1 : 2;",
   "true ?? 1 : 2;",
   "true (? 1 :) 2;",
   "true (?:) 2;",
   "true (? false ? 1 : 2): 3;",
   "true ? (false ? 1 : 2:) 3;",
   "(true ? false ? 1 : 2): 3;",

    // invalid crement
    "++5;",
    "5++;",
    "--5;",
    "5--;",
    "++5--;",
    "++5++;",
    "--5++;",
    "--5--;",
    "{ 1, 1, 1}++;",
    "++{ 1, 1, 1};",
    "--{ 1, 1, 1};",
    "{ 1, 1, 1}--;",
    "++{ 1, 1, 1}++;",
    "++{ 1, 1, 1}--;",
    "--{ 1, 1, 1}--;",
    "++a-;",
    //"++a--;",
    //"++a++;",
    //"--a++;",
    //"--a--;",
    //"----a;",
    //"++++a;",
    //"a.x--;",
    //"-a.y--;",
    //"++a.z;",
    //"++@a--;",
    //"@a.x--;",
    //"-@a.y--;",
    //"++@a.z;",
    "++$a--;",
    "$a.x--;",
    "-$a.y--;",
    "++$a.z;",
    "--f();",
    "f()++;",
    "return++;",
    "--return;",
    "true++;",
    "--false;",
    "--if;",
    "if++;",
    "else++;",
    "--else;",
    "--bool;",
    "short++;",
    "--int;",
    "long++;",
    "--float;",
    "++double;",
    "--vector;",
    "matrix--;",
    "--();",
    "()++;",
    "{}++;",
    "--{};",
    "--,;",
    ",--;",

    // invalid declare
    "int;",
    "int 1;",
    "string int;",
    "int bool a;",
    "int a",
    "vector a",
    "vector float a",

    // invalid function
    "function(;",
    "function);",
    "return();",
    "function(bool);",
    "function(bool a);",
    "function(+);",
    "function(!);",
    "function(~);",
    "function(-);",
    "function(&&);",
    "function{};" ,
    "function(,);" ,
    "function(, a);",
    "function(a, );",
    "function({,});",
    "function({});",
    "function({)};",
    "function{()};",
    "function{(});",
    "function{,};",
    "function(if(true) {});",
    "function(return);",
    "function(return;);",
    "function(while(true) a);",
    "function(while(true) {});",
    "\"function\"();" ,
    "();",
    "+();",
    "10();",

    // invalid keyword return
    "return",
    "int return;",
    "return return;",
    "return max(1, 2);",
    "return 1 + a;",
    "return !a;",
    "return a = 1;",
    "return a.x = 1;",
    "return ++a;",
    "return int(a);",
    "return {1, 2, 3};",
    "return a[1];",
    "return true;",
    "return 0;",
    "return (1);",
    "return \"a\";",
    "return int a;",
    "return a;",
    "return @a;",

    // invalid unary
    "+bool;" ,
    "+bool a;" ,
    "bool -a;" ,
    "-return;" ,
    "!return;" ,
    "+return;" ,
    "~return;" ,
    "~if(a) {};" ,
    "if(a) -{};" ,
    "if(a) {} !else {};",
    // @todo  unary crementation expressions should be parsable but perhaps
    //        not compilable
    "---a;" ,
    "+++a;" ,

    // invalid value
    ".0.0;",
    ".0.0f;",
    ".f;",
    "0..0;",
    "0.0l;",
    "0.0ls;",
    "0.0s;",
    "0.0sf;",
    "0.a",
    "0.af",
    "00ls;",
    "0ef;",
    "0f0;",
    "1.0f.0;",
    "1.\"1\";",
    "1.e6f;",
    "10000000.00000001s;",
    "1e.6f;",
    "1Ee6;",
    "1ee6;",
    "1eE6f;",
    "1ee6f;",
    "1l.0;",
    "1s.0;",
    "\"1.\"2;",
    "a.0",
    "a.0f",
    "false.0;",
    "true.;",

    // invalid vector
    "{1,2,3];",
    "[1,2,3};",
    "{,,};",
    "{,2,3};",
    "{()};",
    "{(1,)};",
    "{(,1)};",
    "{(1});",
    "({1)};",
    "{1,};",
    "{,1};",

    // invalid vector unpack
    "5.x;",
    "foo.2;",
    "a.w;",
    "a.X;",
    "a.Y;",
    "a.Z;",
    "@a.X;",
    "@a.Y;",
    "@a.Z;",
    "$a.X;",
    "$a.Y;",
    "$a.Z;",
    "a.xx;",
    "a++.x",
    "++a.x",
    "func().x",
    "int(y).x",
    "vector .",
    "vector .x",
    "vector foo.x",
    "(a + b).x",
    "(a).x;",
    "(@a).x;",
    "@.x;",
    "($a).x;",
    "$.x;",
    "true.x;",
    "a.rx;",
    "a.rgb;",

    // other failures (which may be used in the future)
    "function<>();",
    "function<true>();",
    "a[1:1];",
    "a.a;",
    "a->a;",
    "&a;",
    "a . a;",
    "a .* a;",
    "@a();",
    "$a();",
    "@a.function();",
    "@a.member;",
    "/**/;",
    "(a,a,a) = (b,b,b);",
    "(a,a,a) = 1;",
    "(a) = 1;",
    "a = (a=a) = a;",

    // invalid lone characters
    "£;",
    "`;",
    "¬;",
    "@;",
    "~;",
    "+;",
    "-;",
    "*;",
    "/;",
    "<<;",
    ">>;",
    ">;",
    "<;",
    "[];",
    "|;",
    ",;",
    "!;",
    "\\;"
};

}

class TestSyntaxFailures : public CppUnit::TestCase
{
public:

    CPPUNIT_TEST_SUITE(TestSyntaxFailures);
    CPPUNIT_TEST(testSyntax);
    CPPUNIT_TEST_SUITE_END();

    void testSyntax();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSyntaxFailures);

void TestSyntaxFailures::testSyntax()
{
    // Quickly check the above doesn't have multiple occurance
    // store multiple in a hash map
    const auto hash = [](const StrWrapper* s) {
        return std::hash<std::string>()(s->first);
    };
    const auto equal = [](const StrWrapper* s1, const StrWrapper* s2) {
        return s1->first.compare(s2->first) == 0;
    };
    std::unordered_map<const StrWrapper*,
        size_t, decltype(hash), decltype(equal)> map(tests.size(), hash, equal);

    for (const auto& test : tests) {
        ++map[&test];
    }

    // Print strings that occur more than once
    for (auto iter : map) {
        if (iter.second > 1) {
            std::cout << iter.first->first << " printed x" << iter.second << std::endl;
        }
    }

    TEST_SYNTAX_FAILS(tests);
}


// Copyright (c) 2015-2020 DNEG
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
