// Copyright 2007-2008 Google Inc.  All Rights Reserved.

/**
 * @fileoverview Test indexing on strings with [].
 */

var foo = "Foo";
assertEquals("Foo", foo);
assertEquals("F", foo[0]);
assertEquals("o", foo[1]);
assertEquals("o", foo[2]);

assertEquals("F", foo["0" + ""], "string index");
assertEquals("o", foo["1"], "string index");
assertEquals("o", foo["2"], "string index");

assertEquals("undefined", typeof(foo[3]), "out of range");
// SpiderMonkey 1.5 fails this next one.  So does FF 2.0.6.
assertEquals("undefined", typeof(foo[-1]), "known failure in SpiderMonkey 1.5");
assertEquals("undefined", typeof(foo[-2]), "negative index");

foo[0] = "f";
assertEquals("Foo", foo);

foo[3] = "t";
assertEquals("Foo", foo);
assertEquals("undefined", typeof(foo[3]), "out of range");
assertEquals("undefined", typeof(foo[-2]), "negative index");

var S = new String("foo");
assertEquals("foo", S);
assertEquals("f", S[0], "string object");
assertEquals("f", S["0"], "string object");
S[0] = 'bente';
assertEquals("f", S[0], "string object");
assertEquals("f", S["0"], "string object");
S[-2] = 'spider';
assertEquals('spider', S[-2]);
S[3] = 'monkey';
assertEquals('monkey', S[3]);
S['foo'] = 'Fu';
assertEquals("Fu", S.foo);

// In FF this is ignored I think.  In V8 it puts a property on the String object
// but you won't ever see it because it is hidden by the 0th character in the
// string.  The net effect is pretty much the same.
S["0"] = 'bente';
assertEquals("f", S[0], "string object");
assertEquals("f", S["0"], "string object");

assertEquals(true, 0 in S, "0 in");
assertEquals(false, -1 in S, "-1 in");
assertEquals(true, 2 in S, "2 in");
assertEquals(true, 3 in S, "3 in");
assertEquals(false, 4 in S, "3 in");

assertEquals(true, "0" in S, '"0" in');
assertEquals(false, "-1" in S, '"-1" in');
assertEquals(true, "2" in S, '"2" in');
assertEquals(true, "3" in S, '"3" in');
assertEquals(false, "4" in S, '"3" in');

assertEquals(true, S.hasOwnProperty(0), "0 hasOwnProperty");
assertEquals(false, S.hasOwnProperty(-1), "-1 hasOwnProperty");
assertEquals(true, S.hasOwnProperty(2), "2 hasOwnProperty");
assertEquals(true, S.hasOwnProperty(3), "3 hasOwnProperty");
assertEquals(false, S.hasOwnProperty(4), "3 hasOwnProperty");

assertEquals(true, S.hasOwnProperty("0"), '"0" hasOwnProperty');
assertEquals(false, S.hasOwnProperty("-1"), '"-1" hasOwnProperty');
assertEquals(true, S.hasOwnProperty("2"), '"2" hasOwnProperty');
assertEquals(true, S.hasOwnProperty("3"), '"3" hasOwnProperty');
assertEquals(false, S.hasOwnProperty("4"), '"3" hasOwnProperty');

assertEquals(true, "foo".hasOwnProperty(0), "foo 0 hasOwnProperty");
assertEquals(false, "foo".hasOwnProperty(-1), "foo -1 hasOwnProperty");
assertEquals(true, "foo".hasOwnProperty(2), "foo 2 hasOwnProperty");
assertEquals(false, "foo".hasOwnProperty(4), "foo 3 hasOwnProperty");

assertEquals(true, "foo".hasOwnProperty("0"), 'foo "0" hasOwnProperty');
assertEquals(false, "foo".hasOwnProperty("-1"), 'foo "-1" hasOwnProperty');
assertEquals(true, "foo".hasOwnProperty("2"), 'foo "2" hasOwnProperty');
assertEquals(false, "foo".hasOwnProperty("4"), 'foo "3" hasOwnProperty');

//assertEquals(true, 0 in "foo", "0 in");
//assertEquals(false, -1 in "foo", "-1 in");
//assertEquals(true, 2 in "foo", "2 in");
//assertEquals(false, 3 in "foo", "3 in");
//
//assertEquals(true, "0" in "foo", '"0" in');
//assertEquals(false, "-1" in "foo", '"-1" in');
//assertEquals(true, "2" in "foo", '"2" in');
//assertEquals(false, "3" in "foo", '"3" in');

delete S[3];
assertEquals("undefined", typeof(S[3]));
assertEquals(false, 3 in S);
assertEquals(false, "3" in S);

var N = new Number(43);
assertEquals(43, N);
N[-2] = "Alpha";
assertEquals("Alpha", N[-2]);
N[0] = "Zappa";
assertEquals("Zappa", N[0]);
assertEquals("Zappa", N["0"]);

var A = ["V", "e", "t", "t", "e", "r"];
var A2 = (A[0] = "v");
assertEquals('v', A[0]);
assertEquals('v', A2);

var S = new String("Onkel");
var S2 = (S[0] = 'o');
assertEquals('O', S[0]);
assertEquals('o', S2);

var s = "Tante";
var s2 = (s[0] = 't');
assertEquals('T', s[0]);
assertEquals('t', s2);

var S2 = (S[-2] = 'o');
assertEquals('o', S[-2]);
assertEquals('o', S2);

var s2 = (s[-2] = 't');
assertEquals('undefined', typeof(s[-2]));
assertEquals('t', s2);
