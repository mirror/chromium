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
// From within 'eval', the name of the enclosing function should be
// visible.

var f_t = function y() { return typeof y; }
check(f_t(), "function");


var f_e = function y() { return eval('typeof y'); }
check(f_e(), "function");


var fat0 = function y() { y = 3; return typeof y; }
check(fat0(), "function");


var fat1 = function y() { y += 3; return typeof y; }
check(fat1(), "function");


var fat2 = function y() { y &= y; return typeof y; }
check(fat2(), "function");


var fae = function y() { y = 3; return eval('typeof y'); }
check(fae(), "function");


var fdt = function y() { var y = 3; return typeof y; }
check(fdt(), "number");


var fde = function y() { var y = 3; return eval('typeof y'); }
check(fde(), "number");


var feat = function y() { eval('y = 3'); return typeof y; }
check(feat(), "function");


var feae = function y() { eval('y = 3'); return eval('typeof y'); }
check(feae(), "function");


var fedt = function y() { eval('var y = 3'); return typeof y; }
check(fedt(), "number");


var fede = function y() { eval('var y = 3'); return eval('typeof y'); }
check(fede(), "number");


// ----------------------------------------------------------------------------
test_report();
