// Copyright 2008 Google Inc.  All Rights Reserved.

/**
 * @fileoverview Check that flattening deep trees of cons strings does not
 * cause stack overflows.
 */

var depth = 110000;

function newdeep(start) {
  var d = start;
  for (var i = 0; i < depth; i++) {
    d = d + "f";
  }
  return d;
}

var deep = newdeep("foo");
assertEquals('f', deep[0]);

var cmp1 = newdeep("a");
var cmp2 = newdeep("b");

assertEquals(-1, cmp1.localeCompare(cmp2), "ab");

var cmp2empty = newdeep("c");
assertTrue(cmp2empty.localeCompare("") > 0, "c");

var cmp3empty = newdeep("d");
assertTrue("".localeCompare(cmp3empty) < 0), "d";

var slicer = newdeep("slice");

for (i = 0; i < depth + 4; i += 2) {
  slicer =  slicer.slice(1, -1);
}

assertEquals("f", slicer[0]);
assertEquals(1, slicer.length);
