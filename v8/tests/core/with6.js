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
// The following test cases fail with an older (buggy) implementation
// of lazy compilation.

var new_inner_functions_semantics = true;

try {
  var yyy = 0;
  with ({ yyy: 42 }) {
    function bar() {
      return yyy;
    }
  }
} catch (e) {
  check(0, 1)
}

check(bar(), (new_inner_functions_semantics ? 0 : 42));


try {
  var y = 0;
  var obj = { y: 10 };
  with (obj) {
    function f() { return y; }
    g = function() { return y; }
  }
} catch (e) {
  check(0, 1)
}


check(f(), (new_inner_functions_semantics ? 0 : 10));
check(g(), 10);


// ----------------------------------------------------------------------------

test_report();
