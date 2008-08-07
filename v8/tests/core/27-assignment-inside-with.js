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
// An assignment inside a 'with' statement must change the
// existing slot, if in the actual object. However, it must create
// a new slot in the actual object if the property was found in
// a prototype.

function Foo(x) {
  this.foo = x;
}

Foo.prototype.bar = "bar";
Foo.prototype.Check = function (f, b) {
  check(this.foo, f);
  check(this.bar, b);
}

var obj1 = new Foo("foo1");
var obj2 = new Foo("foo2");

obj1.Check("foo1", "bar");
obj2.Check("foo2", "bar");

with (obj1) {
  bar = "bra";
}

obj1.Check("foo1", "bra");
obj2.Check("foo2", "bar");


// ----------------------------------------------------------------------------
//

test_report();
