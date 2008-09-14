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
//

var new_inner_functions_semantics = true;

function f0() {
  var a = "a";
  var b = "b";
  var c = "c";
  
  var x = { a: "x.a", b: "x.b" };
  with (x) {
    function inner_fa()  { return a; }
    function inner_fb()  { return b; }
    function inner_fc()  { return c; }
  }
  
  x.b = "new x.b";
  
  check(inner_fa(), (new_inner_functions_semantics ? "a" : "x.a"));
  check(inner_fb(), (new_inner_functions_semantics ? "b" : "new x.b"));
  check(inner_fc(), "c");
}

f0();

// ----------------------------------------------------------------------------
test_report();
