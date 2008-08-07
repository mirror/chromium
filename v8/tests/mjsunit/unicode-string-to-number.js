// Make sure not to treat 16-bit unicode characters as ASCII
// characters when converting to numbers.
assertEquals(272, Number('2\u00372'));
assertTrue(isNaN(Number('2\u11372')), "short-string");

// Check that long string can convert to proper numbers.
var s = '\u0031';
for (var i = 0; i < 7; i++) {
  s += s;
}
assertTrue(isFinite(Number(s)));

// Check that long strings with non-ASCII characters cannot convert.
var s = '\u1131';
for (var i = 0; i < 7; i++) {
  s += s;
}
assertTrue(isNaN(Number(s)), "long-string");

