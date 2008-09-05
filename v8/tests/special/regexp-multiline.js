/*
 * This file is included in all mini jsunit test cases.  The test
 * framework expects lines that signal failed tests to start with
 * the f-word and ignore all other lines.
 */

// Avoid writing the f-word, since some tests output parts of this code.
var the_f_word = "Fai" + "lure";

function fail(expected, found, name_opt) {
  var start;
  if (name_opt) {
    start = the_f_word + " (" + name_opt + "): ";
  } else {
    start = the_f_word + ":";
  }
  print(start + " expected <" + expected + "> found <" + found + ">");
}


function assertEquals(expected, found, name_opt) {
  if (expected != found) {
    fail(expected, found, name_opt);
  }
}


function assertArrayEquals(expected, found, name_opt) {
  var start = "";
  if (name_opt) {
    start = name_opt + " - ";
  }
  assertEquals(expected.length, found.length, start + "array length");
  if (expected.length == found.length) {
    for (var i = 0; i < expected.length; ++i) {
      assertEquals(expected[i], found[i], start + "array element at index " + i);
    }
  }
}


function assertTrue(value, name_opt) {
  assertEquals(true, value, name_opt);
}


function assertFalse(value, name_opt) {
  assertEquals(false, value, name_opt);
}


function assertNaN(value, name_opt) {
  if (!isNaN(value)) {
    fail("NaN", value, name_opt);
  }
}


function assertThrows(code) {
  try {
    eval(code);
    assertTrue(false, "did not throw exception");
  } catch (e) {
    // Do nothing.
  }
}


function assertInstanceof(obj, type) {
  if (!(obj instanceof type)) {
    assertTrue(false, "Object <" + obj + "> is not an instance of <" + type + ">");
  }
}


function assertDoesNotThrow(code) {
  try {
    eval(code);
  } catch (e) {
    assertTrue(false, "threw an exception");
  }
}


function assertUnreachable(name_opt) {
  var message = the_f_word + ": unreachable"
  if (name_opt) {
    message += " - " + name_opt;
  }
  print(message);
}

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
