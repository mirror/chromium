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
// Multiply declared parameters must be properly accessed even from
// within 'eval()' and 'with'.

(function() {
  function f(x, y, x) {
    eval("check(x, 3)");
    with ({}) {
      check(x, 3);
    }
    check(x, 3);
  }
  f(1, 2, 3);
})();

// ----------------------------------------------------------------------------

test_report();
