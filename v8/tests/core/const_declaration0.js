// Copyright 2007-2008 Google Inc.  All Rights Reserved.

// ----------------------------------------------------------------------------
// Testing support

var tests = 0;
var errors = 0;
var comment = "";

function set_test(s) {
  comment = " in " + s;
}

function check(found, expected) {
  tests++;
  if (found != expected) {
    print("error" + comment + ": found " + found + ", expected: " + expected);
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


// TODO: GET RID OF THIS!
function disabled_check() {
}


// ----------------------------------------------------------------------------
// Global and local const declarations.

set_test("global decls");
const x = 0;
const a = 1, b = "foo", c = true;
check(x, 0);
check(a, 1);
check(b, "foo");
check(c, true);


set_test("f0");
function f0() {
  const x = 0;
  const a = 1, b = "foo", c = true;
  check(x, 0);
  check(a, 1);
  check(b, "foo");
  check(c, true);
}

f0();


set_test("fact");
function fact(n) {
  if (n > 0) return n * fact(n - 1);
  return 1;
}

const fact10 = fact(10);
check(fact10, 3628800);


// ----------------------------------------------------------------------------
// "Forward" declarations

set_test("forward decls");
check(forward1, undefined);
const forward1 = "foo1";
check(forward1, "foo1");

check(forward2, undefined);
forward2 = "changed0";
disabled_check(forward2, undefined);
const forward2 = "foo2";
check(forward2, "foo2");


set_test("f3");
function f3() {

  forward3 = "baa";
  function inner1() {
    check(forward1, undefined);
    const forward1 = "foo1";
    check(forward1, "foo1");
    
    check(forward2, undefined);
    forward2 = "changed2";
    check(forward2, undefined);
    const forward2 = "foo2";
    check(forward2, "foo2");
    
    check(forward3, undefined);
    forward3 = "changed3";
    check(forward3, undefined);
    const forward3 = "foo3";
    check(forward3, "foo3");
  }
  inner1();

  check(forward4, undefined);
  function inner2() {
    check(forward4, undefined);
    forward4 = "changed4";
    check(forward4, undefined);
  }
  inner2();
  const forward4 = "foo4";
  check(forward4, "foo4");
}

f3();


// ----------------------------------------------------------------------------
// Check that const cannot be reassigned.

set_test("re-assignment");
check(reassign1, undefined);
const reassign1 = 1;
reassign1 = 2;
disabled_check(reassign1, 1);


function const1() {
  const x;
  x = 3;
  return x;
}

function const2() {
  const x = 2;
  x = 3;
  return x;
}

check(const1(), undefined);
check(const2(), 2);


// ----------------------------------------------------------------------------
// The right side of const is always evaluated.

set_test("RHS always evaluated");

var sum = 0;

function f1() { return ++sum; }

function const3() {
  for (i = 0; i < 3; i++) {
    const x = f1();
    x = f1();
  }
  return x;
}

check(const3(), 1);
check(sum, 6);


// ----------------------------------------------------------------------------
// const declarations that contain an initializer are not syntactic
// sugar for a two-step initialization (as it is the case for variables).

set_test("const and with");

z = 0;
disabled_check(z, undefined);
with ({ z: -2 }) {
  const z = 2;
  disabled_check(z, -2);  // this doesn't change the object's z!
}
disabled_check(z, 2);

function f2() {
  var x = 0;
  with ({ x: -1 }) {
    var x = 1;  // shortcut for var x; x = 1; (this changes the object's x)
    check(x, 1);
  }
  check(x, 0);

  with ({ y: -1 }) {
    const y = 1;  // this doesn't change the object's y!
    check(y, -1);
  }
  check(y, 1);
}

f2();


// ----------------------------------------------------------------------------
// const declarations through eval() calls must work as expected.

set_test("const and eval");

var ss = "const xx = 0;";
var tt = "xx = 1;";

eval(ss);
eval(tt);
xx = 2;
disabled_check(xx, 0);


function f4(s, t) {
  eval(s);
  eval(t);
  xx = 2;
  disabled_check(xx, 0);
}

f4(ss, tt);


// ----------------------------------------------------------------------------
// const declarations can be used in for loops

set_test("const and for");

function f5(n) {
  for (const index1 = 0; index1 < n; n--) {
    index1 = n;
    check(index1, 0);
  }
  check(index1, 0);

  for (var i = 0; i < 3; i++) {
    for (const index2 = i; index2 < n; n--) {
      index2 = n;
      check(index2, 0);
    }
    check(index2, 0);
  }
  check(index2, 0);
}

f5(10);


// ----------------------------------------------------------------------------
test_report();
