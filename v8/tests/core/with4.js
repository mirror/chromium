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
// Tests dynamic variable accesses accross several scopes with
// a 'with' statement inbetween. The old variable resolution code
// used to incorrectly terminate early and not mark the parameter
// 'x' as potentially accessed from an inner scope. As a result, the
// access of 'x' inside 'foobar' resulted in a reference error.

function foo(x) {
  
  function bar() {
    with ({}) {
      function foobar() {
        return x;
      }
    }
    
    return foobar();
  }
  
  return bar();
}

check(foo(3), 3);


// ----------------------------------------------------------------------------

test_report();
