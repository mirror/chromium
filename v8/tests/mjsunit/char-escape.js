// Check that character escapes are understood as one char
var escapes = ["\b", "\t", "\n", "\v", "\f", "\r", "\"", "\'", "\\", "\x4a", "\u005f"];
for (var i = 0; i < escapes.length; i++) {
  var str = escapes[i];
  assertEquals(1, str.length);
  assertEquals(str, str.charAt(0));
}

function code(str) {
  return str.charCodeAt(0);
}

// Do the single escape chars have the right value?
assertEquals(0x08, code("\b"));
assertEquals(0x09, code("\t"));
assertEquals(0x0A, code("\n"));
assertEquals(0x0B, code("\v"));
assertEquals(0x0C, code("\f"));
assertEquals(0x0D, code("\r"));
assertEquals(0x22, code("\""));
assertEquals(0x27, code("\'"));
assertEquals(0x5c, code("\\"));

// Do the hex and unicode escape chars have the right value?
assertEquals(0x4a, code("\x4a"));
assertEquals(0x5f, code("\u005f"));

// TODO(plesner): Add tests for longer unicode escapes
