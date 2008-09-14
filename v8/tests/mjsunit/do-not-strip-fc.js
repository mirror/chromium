// Make sure we do not remove unicode format-control characters
// from string literals.
assertEquals(7, eval("'foo\u200dbar'").length);
assertEquals(7, eval("'foo\u200cbar'").length);
