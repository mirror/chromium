// ----------------------------------------------------------------------------
// Testing support

var errors = 0;

function check(found, expected) {
  if (found != expected) {
    print("error: found " + found + ", expected: " + expected);
    errors++;
  }
}

function test_report() {
  if (errors > 0) {
    print("FAILED (" + errors + " errors)");
  } else {
    print("PASSED")
  }
}


// ----------------------------------------------------------------------------
// Tests that access parameters copied into eval contexts.

// 1
function f1(x) {
  return eval('x');
}

check(f1(3), 3);


// 2
check(eval("(function (x) { return eval('x'); })")(5), 5);


// 3
f3 = new Function("x", "return eval('x++');");
check(f3(7), 7);


// 4
function f4(x) {
  eval('x++;');
  return x;
}

check(f4(3), 4);


// 5
check(eval("(function (x) { eval('x++'); return x;})")(5), 6);


// 6
f6 = new Function("x", "eval('x++'); return x;");
check(f6(7), 8);


test_report();
