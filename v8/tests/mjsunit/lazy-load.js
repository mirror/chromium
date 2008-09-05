// Test unusual way of accessing Date.
var date0 = new this["Date"](1111);
assertEquals(1111, date0.getTime());

// Check that regexp literals use original RegExp (non-ECMA-262).
RegExp = 42;
var re = /test/;
