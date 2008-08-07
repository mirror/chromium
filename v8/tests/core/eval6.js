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
// A var declaration in an eval scope must lead to a property creation
// in the surrounding context even if the variable doesn't have an initialization
// expression!

eval("var b = 2;");
check(b, 2);


eval("var c;");
check(c, undefined);


function f0() {
  eval("var a = 3;");
  return a;
}

check(f0(), 3);


function f1() {
  a = 5;
  eval("var a;");
  return a;
}

check(f1(), undefined);


function f2(p, s) {
  var y = 0;
  function g() {
    y = 5;
    eval(s);
    y = 7;
  }
  g();
  return y;
}

check(f2(true, "if (p) var y;"), 5);

check(f2(false, ""), 7);


// ----------------------------------------------------------------------------

test_report();
