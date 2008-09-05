var s = "test test test";

assertEquals(5, s.indexOf("test", 4));
assertEquals(5, s.indexOf("test", 5));
assertEquals(10, s.indexOf("test", 6));
assertEquals(0, s.indexOf("test", 0));
assertEquals(0, s.indexOf("test", -1));
assertEquals(0, s.indexOf("test"));
assertEquals(-1, s.indexOf("notpresent"));
assertEquals(-1, s.indexOf());

for (var i = 0; i < s.length+10; i++) {
  var expected = i < s.length ? i : s.length;
  assertEquals(expected, s.indexOf("", i));
}

var reString = "asdf[a-z]+(asdf)?";

assertEquals(4, reString.indexOf("[a-z]+"));
assertEquals(10, reString.indexOf("(asdf)?"));

assertEquals(1, String.prototype.indexOf.length);
