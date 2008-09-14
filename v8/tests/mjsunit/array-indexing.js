var array = [1,2,3,1,2,3,1,2,3,1,2,3];

// ----------------------------------------------------------------------
// Array.prototype.indexOf.
// ----------------------------------------------------------------------

// Negative cases.
assertEquals([].indexOf(1), -1);
assertEquals(array.indexOf(4), -1);
assertEquals(array.indexOf(3, array.length), -1);

assertEquals(array.indexOf(3), 2);
// Negative index out of range.
assertEquals(array.indexOf(1, -17), 0);
// Negative index in rage.
assertEquals(array.indexOf(1, -11), 3);
// Index in range.
assertEquals(array.indexOf(1, 1), 3);
assertEquals(array.indexOf(1, 3), 3);
assertEquals(array.indexOf(1, 4), 6);

// ----------------------------------------------------------------------
// Array.prototype.lastIndexOf.
// ----------------------------------------------------------------------

// Negative cases.
assertEquals([].lastIndexOf(1), -1);
assertEquals(array.lastIndexOf(1, -17), -1);

assertEquals(array.lastIndexOf(1), 9);
// Index out of range.
assertEquals(array.lastIndexOf(1, array.length), 9);
// Index in range.
assertEquals(array.lastIndexOf(1, 2), 0);
assertEquals(array.lastIndexOf(1, 4), 3);
assertEquals(array.lastIndexOf(1, 3), 3);
// Negative index in range.
assertEquals(array.lastIndexOf(1, -11), 0);

