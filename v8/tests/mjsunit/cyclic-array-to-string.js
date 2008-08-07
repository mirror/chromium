// Copyright 2008 Google Inc. All Rights Reserved.

// Test printing of cyclic arrays.

var a1 = [1,2];
assertEquals("1,2", a1.toString());
assertEquals("1,2", a1.toLocaleString());
assertEquals("1,2", a1.join());
a1.push(a1);
assertEquals("1,2,", a1.toString());
assertEquals("1,2,", a1.toLocaleString());
assertEquals("1,2,", a1.join());
a1.push(1);
assertEquals("1,2,,1", a1.toString());
assertEquals("1,2,,1", a1.toLocaleString());
assertEquals("1,2,,1", a1.join());
a1.push(a1);
assertEquals("1,2,,1,", a1.toString());
assertEquals("1,2,,1,", a1.toLocaleString());
assertEquals("1,2,,1,", a1.join());

a1 = [1,2];
var a2 = [3,4];
a1.push(a2);
a1.push(a2);
assertEquals("1,2,3,4,3,4", a1.toString());
assertEquals("1,2,3,4,3,4", a1.toLocaleString());
assertEquals("1,2,3,4,3,4", a1.join());
a2.push(a1);
assertEquals("1,2,3,4,,3,4,", a1.toString());
assertEquals("1,2,3,4,,3,4,", a1.toLocaleString());
assertEquals("1,2,3,4,,3,4,", a1.join());

a1 = [];
a2 = [a1];
a1.push(a2);
assertEquals("", a1.toString());
assertEquals("", a1.toLocaleString());
assertEquals("", a1.join());

