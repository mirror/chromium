var s = "test";

assertEquals("t", s.charAt());
assertEquals("t", s.charAt("string"));
assertEquals("t", s.charAt(null));
assertEquals("t", s.charAt(void 0));
assertEquals("t", s.charAt(false));
assertEquals("e", s.charAt(true));
assertEquals("", s.charAt(-1));
assertEquals("", s.charAt(4));
assertEquals("t", s.charAt(0));
assertEquals("t", s.charAt(3));
assertEquals("t", s.charAt(NaN));

assertEquals(116, s.charCodeAt());
assertEquals(116, s.charCodeAt("string"));
assertEquals(116, s.charCodeAt(null));
assertEquals(116, s.charCodeAt(void 0));
assertEquals(116, s.charCodeAt(false));
assertEquals(101, s.charCodeAt(true));
assertEquals(116, s.charCodeAt(0));
assertEquals(116, s.charCodeAt(3));
assertEquals(116, s.charCodeAt(NaN));
assertTrue(isNaN(s.charCodeAt(-1)));
assertTrue(isNaN(s.charCodeAt(4)));

