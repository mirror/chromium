// Flags: --expose-debug-as debug
// Test the mirror object for boolean values

function testBooleanMirror(b) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(b);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.BooleanMirror);

  // Check the mirror properties.
  assertTrue(mirror.isBoolean());
  assertEquals('boolean', mirror.type());
  assertTrue(mirror.isPrimitive());

  // Test text representation
  assertEquals(b ? 'true' : 'false', mirror.toText());

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('boolean', fromJSON.type, json);
  assertEquals(b, fromJSON.value, json);
}


// Test all boolean values.
testBooleanMirror(true);
testBooleanMirror(false);