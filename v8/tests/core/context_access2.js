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
// Test that context lookup is lexical: When accessing fx and gx
// in h (for n == 0), we must get the outermost fx and gx, even
// though there are many n h frames inbetween.

function f(n) {
  var fx = "fx = " + n;
  
  function g(n) {
    var gx = "gx = " + n;
    
    function h(n) {
      if (n > 0) return h(n-1);
      return fx + "; " + gx;
    }
    
    return h(n);
  }
  
  return g(n);
}

check(f(10), "fx = 10; gx = 10");
test_report();
