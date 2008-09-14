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
// According to ECMA-262, sections 8.6.2.2 and 8.6.2.3 you're not
// allowed to override read-only properties.

function F() {};
F.prototype = Number;

var f = new F();
check(Number.MAX_VALUE, f.MAX_VALUE);

with (f) {
  MAX_VALUE = 42;  // should not take effect
}

check(Number.MAX_VALUE, f.MAX_VALUE);


// G should be read-only.
(function G() {
  eval("G = 42;");
  check(typeof G === 'function', true);
})();


// ----------------------------------------------------------------------------

test_report();
