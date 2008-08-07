// Flags: --expose-debug-as debug
// Test the mirror object for boolean values

function testDateMirror(d, iso8601) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(d);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.ObjectMirror);
  assertTrue(mirror instanceof debug.DateMirror);

  // Check the mirror properties.
  assertTrue(mirror.isDate());
  assertEquals('object', mirror.type());
  assertFalse(mirror.isPrimitive());

  // Test text representation
  assertEquals(iso8601, mirror.toText());

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('object', fromJSON.type);
  assertEquals('Date', fromJSON.className);
  assertEquals(iso8601, fromJSON.value);
}


// Test Date values.
testDateMirror(new Date(Date.parse("Dec 25, 1995 1:30 UTC")), "1995-12-25T01:30:00.000Z");
d = new Date();
d.setUTCFullYear(1967);
d.setUTCMonth(0); // January.
d.setUTCDate(17);
d.setUTCHours(9);
d.setUTCMinutes(22);
d.setUTCSeconds(59);
d.setUTCMilliseconds(0);
testDateMirror(d, "1967-01-17T09:22:59.000Z");
d.setUTCMilliseconds(1);
testDateMirror(d, "1967-01-17T09:22:59.001Z");
d.setUTCMilliseconds(12);
testDateMirror(d, "1967-01-17T09:22:59.012Z");
d.setUTCMilliseconds(123);
testDateMirror(d, "1967-01-17T09:22:59.123Z");
