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
// Here are some more examples that show the differences between variable
// and function declarations, function declarations and function expressions,
// and function declarations treated like function expressions.

// Top-level functions can be accessed before they are declared.
// They are source elements and translated into variable declarations that
// are initialized upon activation entry.
//
// With the new inner functions semantics, all function declarations are
// considered source elements.

var new_inner_functions_semantics = true;

function f1() {
  check(g1(), "f1: g1");
  function g1() { return "f1: g1"; }
}

f1();


// Functions inside statements cannot be called before they are declared.
// They are not source elements and thus are treated like function expressions.
function f2() {
  check(g1, (new_inner_functions_semantics ? "function g1() { return \"f2: g1\"; }" : undefined));
  do {
    function g1() { return "f2: g1"; }
  } while (false);
  check(g1(), "f2: g1");
}

f2();


// Functions inside statements are in fact initialized dynamically.
// (In SpiderMonkey they are even declared dynamically).
function f3() {
  for (i = 0; i < 2; i++) {
    if (i == 0) check(g1, (new_inner_functions_semantics ? "function g1() { return \"f3: g1\"; }" : undefined));
    if (i == 1) check(g1(), "f3: g1");
    function g1() { return "f3: g1"; }
  }
}

f3();


// Functions inside statements are created using the proper context.
function f3b() {
  var a = "f3b: a";
  var x = new Object();
  x.a = "f3b: x.a";
  with (x) {
    function g1() { return a; }
  }
  check(g1(), (new_inner_functions_semantics ? "f3b: a" : "f3b: x.a"));
}

f3b();


// Global functions inside statements are declared dynamically.
check(f3c, (new_inner_functions_semantics ? "function f3c() { return \"f3c\"; }" : undefined));
do {
  function f3c() { return "f3c"; }
} while (false);
check(f3c(), "f3c");


// Global functions at the top-level are declared "statically" (initialized
// before the code is run).
check(f3d(), "f3d");
function f3d() { return "f3d"; }


// Top-level variables can be accessed before they are declared.
function f4() {
  check(v, undefined);
  var v = "f4: v";
  check(v, "f4: v");
}

f4();


// Variables inside statements can be accessed before they are declared.
function f5() {
  check(v, undefined);
  do {
    var v = "f5: v";
  } while (false);
  check(v, "f5: v");
}

f5();


// For non-source element "function declarations", which are treated "like"
// function expressions, the function name ('g' in this case) is actually not
// part of the function (as would be the case for function expressions) but
// declared outside: When the value of 'g' is changed outside, inside the
// function we also see the changed value (and not the constant value of
// the function name, if it were present).
function f6() {
  if (true) {
    function g() { return g; }
  }
  var g0 = g;
  g = "not g anymore";
  check(g0(), "not g anymore");
}

f6();


// ----------------------------------------------------------------------------

test_report();
