// Copyright 2008 Google Inc. All Rights Reserved.

var alias = eval;
function e(s) { return alias(s); }

assertEquals(42, e("42"));
assertEquals(Object, e("Object"));
assertEquals(e, e("e"));

var caught = false;
try {
  e('s');  // should throw exception since aliased eval is global
} catch (e) {
  caught = true;
  assertTrue(e instanceof ReferenceError);
}
assertTrue(caught);
