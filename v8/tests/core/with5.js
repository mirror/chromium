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
// This code failed with an exception because the parameter of x
// of the closure passed to each was not statically resolved for the
// return statement of that closure due to the outer 'with' statement.
// The bug was imprecise handling of the 'scope_inside_with_statement_'
// flag in scopes.cc; specifically, FunctionScope::LookupRecursive().

function each(f) {
  return f(42);
}

function test0() {
  with (0) {
    try {
      return each(function (x) { return x; });
    } catch (e) {
      return e;
    }
  }
}

check(test0(), 42);


// ----------------------------------------------------------------------------
// The codes below failed because yyy was assumed global instead
// of local (due to a non-sufficient test in FunctionScope::ResolveLocals).

(function test() {
  try {
    var yyy = 42;
    with (0) {
      (function foo() {
          (function bar() { check(yyy, 42) })();
      })();
    }
  } catch (e) {
    check(0, 1);
  }
})();


(function test() {
  try {
    with (0) {
      var yyy = 42;
      (function foo() {
          (function bar() { check(yyy, 42) })();
      })();
    }
  } catch (e) {
    check(0, 1);
  }
})();


// ----------------------------------------------------------------------------

test_report();
