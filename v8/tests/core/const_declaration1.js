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
// Test handling of const variables in various settings.

(function () {
  function f() {
    function g() {
      x = 42;  //  should be ignored
      return x;  // force x into context
    }
    x = 43;  // should be ignored
    check(g(), undefined);
    x = 44;  // should be ignored
    const x = 0;
    x = 45;  // should be ignored
    check(g(), 0);
  }
  f();
})();


(function () {
  function f() {
    function g() {
      with ({foo: 0}) {
        x = 42;  //  should be ignored
        return x;  // force x into context
      }
    }
    x = 43;  // should be ignored
    check(g(), undefined);
    x = 44;  // should be ignored
    const x = 0;
    x = 45;  // should be ignored
    check(g(), 0);
  }
  f();
})();


(function () {
  function f() {
    function g(s) {
      eval(s);
      return x;  // force x into context
    }
    x = 43;  // should be ignored
    check(g("x = 42;"), undefined);
    x = 44;  // should be ignored
    const x = 0;
    x = 45;  // should be ignored
    check(g("x = 46;"), 0);
  }
  f();
})();


(function () {
  function f() {
    function g(s) {
      with ({foo: 0}) {
        eval(s);
        return x;  // force x into context
      }
    }
    x = 43;  // should be ignored
    check(g("x = 42;"), undefined);
    x = 44;  // should be ignored
    const x = 0;
    x = 45;  // should be ignored
    check(g("x = 46;"), 0);
  }
  f();
})();


(function () {
  function f(s) {
    function g() {
      x = 42;  // assign to global x, or to const x
      return x;
    }
    x = 43;  // declare global x
    check(g(), 42);
    x = 44;  // assign to global x
    eval(s);
    x = 45;  // should be ignored (assign to const x)
    check(g(), 0);
  }
  f("const x = 0;");
})();


(function () {
  function f(s) {
    function g() {
      with ({foo: 0}) {
        x = 42;  // assign to global x, or to const x
        return x;
      }
    }
    x = 43;  // declare global x
    check(g(), 42);
    x = 44;  // assign to global x
    eval(s);
    x = 45;  // should be ignored (assign to const x)
    check(g(), 0);
  }
  f("const x = 0;");
})();


(function () {
  function f(s) {
    function g(s) {
      eval(s);
      return x;
    }
    x = 43;  // declare global x
    check(g("x = 42;"), 42);
    x = 44;  // assign to global x
    eval(s);
    x = 45;  // should be ignored (assign to const x)
    check(g("x = 46;"), 0);
  }
  f("const x = 0;");
})();


(function () {
  function f(s) {
    function g(s) {
      with ({foo: 0}) {
        eval(s);
        return x;
      }
    }
    x = 43;  // declare global x
    check(g("x = 42;"), 42);
    x = 44;  // assign to global x
    eval(s);
    x = 45;  // should be ignored (assign to const x)
    check(g("x = 46;"), 0);
  }
  f("const x = 0;");
})();


// ----------------------------------------------------------------------------

test_report();
