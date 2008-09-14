// ----------------------------------------------------------------------------
// Testing support

var tests = 0;
var errors = 0;

function check(found, expected) {
  tests++;
  if (found != expected) {
    print("error: found " + found + ", expected: " + expected);
    errors++;
  }
}

function test_report() {
  if (errors > 0) {
    print("FAILED (" + errors + " errors in " + tests + " tests)");
  } else {
    print("PASSED (" + tests + " tests)")
  }
}


// ----------------------------------------------------------------------------
// A collection of tests, somewhat randomly constructed. Some of
// these tests have similar versions in tests/core/with0.js, or
// tests/core/with1.js, and the last larger function (h) is also
// represented in tests/core/with_function_context.js. rhino
// does not run all tests, and smjs does not appear to do the
// correct thing for the last function. v8 seems to handle it
// correctly now.

// ecma/ExecutionContexts/10.1.4-1.js
// ecma/ExecutionContexts/10.1.4-10.js
// ecma/ExecutionContexts/10.1.4-2.js
// ecma/ExecutionContexts/10.1.4-3.js
// ecma/ExecutionContexts/10.1.4-4.js
// ecma/ExecutionContexts/10.1.4-5.js
// ecma/ExecutionContexts/10.1.4-6.js
// ecma/ExecutionContexts/10.1.4-7.js
// ecma/ExecutionContexts/10.1.4-8.js
// ecma/ExecutionContexts/10.2.2-2.js
//
// ecma/GlobalObject/15.1.2.1-2.js


var new_inner_functions_semantics = true;

function f0() {
  with ({ g: "g" }) {
    function g() {}
    check(g, "g");
  }
  check(g, "function g() {}");
}

f0();


function f1(s) {
  with ({ g: "g" }) {
    eval(s);
    check(g, 2);
  }
  check(g, undefined);
}

f1("var g = 2;");


function f2(s) {
  with ({ g: "g" }) {
    eval(s);
    check(g, "g");
  }
  check(g, "function g() {}");
}

f2("function g() {}");


function f3() {
  var x = new Object()
  x.f = 2
  with (x) {
    function f() { return 42; }
    check(f, 2);
  }
  check(x.f, 2);
  check(f, "function f() { return 42; }");
}

f3();


function f4() {
  var x = new Object()
  x.a = 2
  x.b = 3
  x.c = 0;
  with (x) {
    c = a + b;
    check("x.c inside  'with': " + x.c, "x.c inside  'with': 5");
  }
  check("x.c outside 'with': " + x.c, "x.c outside 'with': 5");
}

f4();


function f5() {
  var x = new Object()
  x.f = 2
  with (x) {
    function f() { return 42; }
    check(f, 2);
  }
  check(x.f, 2);
  check(f, "function f() { return 42; }");
}

f5();


function h() {
  var a = "a";
  var b = "b";
  var c = "c";
  
  var x = new Object()
  x.a = "x.a";
  x.c = "x.c";

  if (new_inner_functions_semantics) {
    check(inner_fa, "function inner_fa() { return a; }");
    check(inner_do, "function inner_do() { return a; }");
  } else {
    // 2 dummy checks to get the same final test count
    check(0, 0);
    check(0, 0);
  }
  check(inner_fc, "function inner_fc() { return c; }");
  check(inner_fax, undefined);
  check(inner_fbx, undefined);
  
  check(inner_fc(), "c");

  with (x) {
    function inner_fa() { return a; }
    function inner_fb() { return b; }
    function inner_fc() { return c; }
    var inner_fax = function() { return a; }
    var inner_fbx = function() { return b; }
  }
  
  do {
    function inner_do() { return a; }
  } while (false);

  x.a = "new x.a";
  x.c = "new x.c";
  function inner_fc() { return c; }
  function outer_fa() { return a; }
  function outer_fb() { return b; }

  check(a, "a");
  check(b, "b");
  check(c, "c");
  
  check(inner_fa(), (new_inner_functions_semantics ? "a" : "new x.a"));
  check(inner_fb(), "b");
  check(inner_fc(), (new_inner_functions_semantics ? "c" : "new x.c"));

  check(inner_fax(), "new x.a");
  check(inner_fbx(), "b");

  check(outer_fa(), "a");
  check(outer_fb(), "b");
}

h();


// ----------------------------------------------------------------------------

test_report();
