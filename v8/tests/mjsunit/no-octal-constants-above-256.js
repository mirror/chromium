// Octal constants above \377 should not be allowed; instead they
// should parse as two-digit octals constants followed by digits.
assertEquals(2, "\400".length);
assertEquals("\40".charCodeAt(0), "\400".charCodeAt(0));
assertEquals("0", "\400".charAt(1));
