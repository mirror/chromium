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


function check_prefix(found, expected_prefix) {
  found = found.toString();
  expected_prefix = expected_prefix.toString();
  tests++;
  if (found.slice(0, expected_prefix.length) != expected_prefix) {
    print("error: found " + found + ", expected string to have prefix: " +
          expected_prefix);
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

function f0() {
  check(f, undefined);
  check(g, undefined);
  with ({ f: "foo", b: "bar" }) {
    var f = 3;
    var g = 5;
    check(f, 3);
    check(g, 5);
  }
  check(f, undefined);
  check(g, 5);
}

function f0m() {
  check(f, undefined);
  check(g, undefined);
  check(h, undefined);
  with ({ f: "foo", b: "bar" }) {
    with ({ g: "goo", c: "car" }) {
      var f = 3;
      var g = 5;
      var h = 7;
      check(f, 3);
      check(g, 5);
      check(h, 7);
    }
  }
  check(f, undefined);
  check(g, undefined);
  check(h, 7);
}

function f0mm() {
  check(f, undefined);
  check(g, undefined);
  check(h, undefined);
  check(i, undefined);
  with ({ f: "foo", b: "bar" }) {
    with ({ g: "goo", c: "car" }) {
      with ({ h: "hoo", d: "dar" }) {
        var f = 3;
        var g = 5;
        var h = 7;
        var i = 11;
        check(f, 3);
        check(g, 5);
        check(h, 7);
        check(i, 11);
      }
    }
  }
  check(f, undefined);
  check(g, undefined);
  check(h, undefined);
  check(i, 11);
}

function f0mmm() {
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
}

f0();
f0m();
f0mm();
f0mmm();


function f1() {
  with ({ f: "foo", b: "bar" }) {
    eval("var f = 3");
    eval("var g = 5");
    check(f, 3);
    check(g, 5);
  }
  check(f, undefined);
  check(g, 5);
}

f1();


function f2() {
  with ({ f: "foo", b: "bar" }) {
    function f() {
};
    function g() {
};
    check(f, "foo");
    check_prefix(g, "function g()");
  }
  check_prefix(f, "function f()");
  check_prefix(g, "function g()");
}

f2();


function f3() {
  with ({ f: "foo", b: "bar" }) {
    eval("function f() {\n};");
    eval("function g() {\n};");
    check(f, "foo");
    check_prefix(g, "function g()");
  }
  check_prefix(f, "function f()");
  check_prefix(g, "function g()");
}

f3();


// ----------------------------------------------------------------------------
test_report();
