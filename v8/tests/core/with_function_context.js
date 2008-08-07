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
// A test to check context-binding of a function inside a 'with'
// statement. SpiderMonkey doesn't seem to do this correctly;
// both v8 and rhino appear to do the right thing.

var new_inner_functions_semantics = true;

function h() {
  var c = "c";
  
  var x = new Object();
  x.a = "x.a";
  x.c = "x.c";

  check(c, "c");
  check(inner_fc, "function inner_fc() { return c; }");
  check(inner_fc(), "c");
  
  with (x) {
    function inner_fc() { var r = c; return r; }
  }

  x.c = "new x.c";
  function inner_fc() { return c; }

  check(c, "c");
  check(inner_fc, (new_inner_functions_semantics
                   ? "function inner_fc() { return c; }"
                   : "function inner_fc() { var r = c; return r; }"));
  check(inner_fc(), (new_inner_functions_semantics ? "c" : "new x.c"));
}

h();


// ----------------------------------------------------------------------------

test_report();
