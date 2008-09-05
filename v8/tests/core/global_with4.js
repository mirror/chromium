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
// Testing a variety of declarations and assignments inside
// 'with' statements.

with ({ f: "foo", b: "bar" }) {
  eval("function f() {\n};");
  eval("function g() {\n};");
  check(f, "foo");
  check(g, "function g() {\n}");
}
check(f, "function f() {\n}");
check(g, "function g() {\n}");


// ----------------------------------------------------------------------------
test_report();
