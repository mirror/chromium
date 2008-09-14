// Flags: --expose-debug-as debug
// Test the mirror object for number values

function testNumberMirror(n) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(n);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.NumberMirror);

  // Check the mirror properties.
  assertTrue(mirror.isNumber());
  assertEquals('number', mirror.type());
  assertTrue(mirror.isPrimitive());

  // Test text representation
  assertEquals(String(n), mirror.toText());

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('number', fromJSON.type);
  if (!isNaN(n)) {
    assertEquals(n, fromJSON.value);
  } else {
    assertTrue(isNaN(fromJSON.value));
  }
}


// Test a number of different numbers.
testNumberMirror(-7);
testNumberMirror(-6.5);
testNumberMirror(0);
testNumberMirror(42);
testNumberMirror(100.0002);
testNumberMirror(Infinity);
testNumberMirror(-Infinity);
testNumberMirror(NaN);
