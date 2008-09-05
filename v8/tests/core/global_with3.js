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
// Testing a variety of declarations and assignments inside
// 'with' statements.

check(f, undefined);
check(g, undefined);
check(h, undefined);
check(i, undefined);
check(j, undefined);
with ({ f: "foo", b: "bar" }) {
  with ({ g: "goo", c: "car" }) {
    with ({ h: "hoo", d: "dar" }) {
      with ({ i: "ioo", e: "ear" }) {
        var f = 3;
        var g = 5;
        var h = 7;
        var i = 11;
        var j = 13;
        check(f, 3);
        check(g, 5);
        check(h, 7);
        check(i, 11);
        check(j, 13);
      }
    }
  }
}
check(f, undefined);
check(g, undefined);
check(h, undefined);
check(i, undefined);
check(j, 13);


// ----------------------------------------------------------------------------
test_report();
