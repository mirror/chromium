// Flags: --expose-debug-as debug
// Test the mirror object for regular error objects

function testErrorMirror(e) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(e);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.ObjectMirror);
  assertTrue(mirror instanceof debug.ErrorMirror);

  // Check the mirror properties.
  assertTrue(mirror.isError());
  assertEquals('error', mirror.type());
  assertFalse(mirror.isPrimitive());
  assertEquals(mirror.message(), e.message, 'source');

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('error', fromJSON.type);
  assertEquals('Error', fromJSON.className);
  assertEquals(fromJSON.message, e.message, 'message');
  
  // Check the formatted text (regress 1231579).
  assertEquals(fromJSON.text, e.toString(), 'toString');
}


// Test Date values.
testErrorMirror(new Error());
testErrorMirror(new Error('This does not work'));
testErrorMirror(new Error(123+456));
testErrorMirror(new EvalError('EvalError'));
testErrorMirror(new RangeError('RangeError'));
testErrorMirror(new ReferenceError('ReferenceError'));
testErrorMirror(new SyntaxError('SyntaxError'));
testErrorMirror(new TypeError('TypeError'));
testErrorMirror(new URIError('URIError'));
