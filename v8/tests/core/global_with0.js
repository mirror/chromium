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

check(f, undefined);
check(g, undefined);
with ({ f: "foo", b: "bar" }) {
  var f = 3;
  var g = 5;
  check(f, 3);
  check(g, 5);
}
check(f, undefined);
check(g, 5);


// ----------------------------------------------------------------------------
test_report();
