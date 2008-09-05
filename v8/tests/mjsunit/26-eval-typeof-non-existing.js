// Typeof expression must resolve to undefined when it used on a
// non-existing property. It is *not* allowed to throw a
// ReferenceError.
assertEquals('undefined', typeof xxx);
assertEquals('undefined', eval('typeof xxx'));
