// Flags: --expose-debug-as debug
// Test the mirror object for string values

function testStringMirror(s) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(s);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.StringMirror);

  // Check the mirror properties.
  assertTrue(mirror.isString());
  assertEquals('string', mirror.type());
  assertTrue(mirror.isPrimitive());

  // Test text representation
  assertEquals(s, mirror.toText());

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('string', fromJSON.type);
  assertEquals(s, fromJSON.value);
}

Number =2;

// Test a number of different strings.
testStringMirror('');
testStringMirror('abcdABCD');
testStringMirror('1234');
testStringMirror('"');
testStringMirror('"""');
testStringMirror("'");
testStringMirror("'''");
testStringMirror("'\"'");
testStringMirror('\\');
testStringMirror('\b\t\n\f\r');
testStringMirror('\u0001\u0002\u001E\u001F');
testStringMirror('"a":1,"b":2');
