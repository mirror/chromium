// Copyright 2008 Google Inc. All Rights Reserved.

// Test that array join calls toString on subarrays.
var a = [[1,2],3,4,[5,6]];
assertEquals('1,2*3*4*5,6', a.join('*'));

// Create a cycle.
a.push(a);
assertEquals('1,2*3*4*5,6*', a.join('*'));

// Replace array.prototype.toString.
Array.prototype.toString = function() { return "array"; }
assertEquals('array*3*4*array*array', a.join('*'));

Array.prototype.toString = function() { throw 42; }
assertThrows("a.join('*')");

Array.prototype.toString = function() { return "array"; }
assertEquals('array*3*4*array*array', a.join('*'));

