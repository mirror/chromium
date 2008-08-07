// Flags: --trace-calls --preallocated-stack-trace-memory 1000000
// Copyright 2008 Google Inc.  All Rights Reserved.

/**
 * @fileoverview Check that various regexp constructs work as intended.
 * Particularly those regexps that use ^ and $.
 */

assertTrue(/^bar/.test("bar"));
assertTrue(/^bar/.test("bar\nfoo"));
assertFalse(/^bar/.test("foo\nbar"));
assertTrue(/^bar/m.test("bar"));
assertTrue(/^bar/m.test("bar\nfoo"));
assertTrue(/^bar/m.test("foo\nbar"));

assertTrue(/bar$/.test("bar"));
assertFalse(/bar$/.test("bar\nfoo"));
assertTrue(/bar$/.test("foo\nbar"));
assertTrue(/bar$/m.test("bar"));
assertTrue(/bar$/m.test("bar\nfoo"));
assertTrue(/bar$/m.test("foo\nbar"));

assertFalse(/^bxr/.test("bar"));
assertFalse(/^bxr/.test("bar\nfoo"));
assertFalse(/^bxr/m.test("bar"));
assertFalse(/^bxr/m.test("bar\nfoo"));
assertFalse(/^bxr/m.test("foo\nbar"));

assertFalse(/bxr$/.test("bar"));
assertFalse(/bxr$/.test("foo\nbar"));
assertFalse(/bxr$/m.test("bar"));
assertFalse(/bxr$/m.test("bar\nfoo"));
assertFalse(/bxr$/m.test("foo\nbar"));


assertTrue(/^.*$/.test(""));
assertTrue(/^.*$/.test("foo"));
assertFalse(/^.*$/.test("\n"));
assertTrue(/^.*$/m.test("\n"));

assertTrue(/^[\s]*$/.test(" "));
assertTrue(/^[\s]*$/.test("\n"));

assertTrue(/^[^]*$/.test(""));
assertTrue(/^[^]*$/.test("foo"));
assertTrue(/^[^]*$/.test("\n"));

assertTrue(/^([()\s]|.)*$/.test("()\n()"));
assertTrue(/^([()\n]|.)*$/.test("()\n()"));
assertFalse(/^([()]|.)*$/.test("()\n()"));
assertTrue(/^([()]|.)*$/m.test("()\n()"));
assertTrue(/^([()]|.)*$/m.test("()\n"));
assertTrue(/^[()]*$/m.test("()\n."));

assertTrue(/^[\].]*$/.test("...]..."));


function check_case(lc, uc) {
  var a = new RegExp("^" + lc + "$");
  assertFalse(a.test(uc));
  a = new RegExp("^" + lc + "$", "i");
  assertTrue(a.test(uc));

  var A = new RegExp("^" + uc + "$");
  assertFalse(A.test(lc));
  A = new RegExp("^" + uc + "$", "i");
  assertTrue(A.test(lc));

  a = new RegExp("^[" + lc + "]$");
  assertFalse(a.test(uc));
  a = new RegExp("^[" + lc + "]$", "i");
  assertTrue(a.test(uc));

  A = new RegExp("^[" + uc + "]$");
  assertFalse(A.test(lc));
  A = new RegExp("^[" + uc + "]$", "i");
  assertTrue(A.test(lc));
}


check_case("a", "A");
// Aring
check_case(String.fromCharCode(229), String.fromCharCode(197));
// Russian G
check_case(String.fromCharCode(0x413), String.fromCharCode(0x433));


assertThrows("a = new RegExp('[z-a]');");
