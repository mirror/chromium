// ----------------------------------------------------------------------------
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
// The arguments object must not exists in eval scopes, but instead
// refer to the enclosing scope.

function f0(x) {
  return eval('arguments[0]');
}

check(f0(7), 7);


var result;
try {
  eval("arguments[0]");
  result = 0;
} catch (x) {
  result = 1;
}
check(result, 1);


// ----------------------------------------------------------------------------
// var declarations inside eval must be 'executed' in the outer scope

function f1(x) {
  var local = x;
  eval("var local = 4;");
  return local;
}

check(f1(3), 4);


function f2(x) {
  eval("var x = 8;");
  return x;
}

check(f2(7), 8);


function f3(x) {
  eval("var y = 11;");
  return y;
}

check(f3(0), 11);


test_report();
