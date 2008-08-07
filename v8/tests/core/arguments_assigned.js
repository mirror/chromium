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
// Assignments to the 'arguments' variable must not invalidate the V8 rewriting
// of parameter accesses.

(function() {
  function f(x) {
    var arguments = [ 1, 2, 3 ];
    return x;
  }
  check(f(7), 7);
})();


(function() {
  function f(x) {
    arguments[0] = 991;
    var arguments = [ 1, 2, 3 ];
    return x;
  }
  check(f(7), 991);
})();


(function() {
  function f(x) {
    arguments[0] = 991;
    for (var i = 0; i < 10; i++) {
      if (i == 5) {
        var arguments = [ 1, 2, 3 ];
      }
    }
    return x;
  }
  check(f(7), 991);
})();


(function() {
  function f(x, s) {
    eval(s);
    return x;
  }
  check(f(7, "var arguments = [ 1, 2, 3 ];"), 7);
})();


(function() {
  function f(x, s) {
    var tmp = arguments[0];
    eval(s);
    return tmp;
  }
  check(f(7, "var arguments = [ 1, 2, 3 ];"), 7);
})();


(function() {
  function f(x, s) {
    var tmp = arguments[0];
    eval(s);
    return tmp;
  }
  check(f(7, ""), 7);
})();


(function() {
  function f(x, s) {
    var tmp = arguments[0];
    eval(s);
    return x;
  }
  check(f(7, "var arguments = [ 1, 2, 3 ];"), 7);
})();


(function() {
  function f(x, s) {
    var tmp = arguments[0];
    eval(s);
    return x;
  }
  check(f(7, ""), 7);
})();


(function() {
  function f(x) {
    function g(y) {
      x = y;
    }
    arguments = {};
    g(991);
    return x;
  }
  check(f(7), 991);
}) ();


(function() {
  function f(x) {
    function g(y, s) {
      eval(s);
    }
    arguments = {};
    g(991, "x = y;");
    return x;
  }
  check(f(7), 991);
}) ();


// ----------------------------------------------------------------------------

test_report();
