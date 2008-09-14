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
// Some basic eval() tests

f0 = new Function("return f0");
check(typeof eval("f0();"), "function");


function f1(x) {
  global_x = 7;
  return h = new Function("return global_x");
}

check(f1(7)(), 7);


function f2(x) {
  return h = new Function("return x");
}

var res = 0;
try {
  f2(7)();
} catch (ex) {
  res |= 1;
} finally {
  res |= 2;
}
check(res, 3);


// Calls of the form eval(x), where x is not a string, must simply
// return x. In particular, x must not be called if it happens to be
// a function (was bug - gri 5/30/07).
function eval_test(x) {
  check(eval(x), x);
}

eval_test(0);
eval_test(3.14);
eval_test(true);
eval_test(undefined);
eval_test(new Object());
eval_test(new Function());
eval_test(function() { print("foo"); });


// ----------------------------------------------------------------------------

test_report();
