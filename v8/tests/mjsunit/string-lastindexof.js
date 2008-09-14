var s = "test test test";

assertEquals(5, s.lastIndexOf("test", 5));
assertEquals(5, s.lastIndexOf("test", 6));
assertEquals(0, s.lastIndexOf("test", 4));
assertEquals(0, s.lastIndexOf("test", 0));
assertEquals(-1, s.lastIndexOf("test", -1));
assertEquals(10, s.lastIndexOf("test"));
assertEquals(-1, s.lastIndexOf("notpresent"));
assertEquals(-1, s.lastIndexOf());
assertEquals(10, s.lastIndexOf("test", "not a number"));

for (var i = s.length + 10; i >= 0; i--) {
  var expected = i < s.length ? i : s.length;
  assertEquals(expected, s.lastIndexOf("", i));
}


var reString = "asdf[a-z]+(asdf)?";

assertEquals(4, reString.lastIndexOf("[a-z]+"));
assertEquals(10, reString.lastIndexOf("(asdf)?"));

assertEquals(1, String.prototype.lastIndexOf.length);
