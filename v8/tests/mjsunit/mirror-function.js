// Flags: --expose-debug-as debug
// Test the mirror object for functions.

function testFunctionMirror(f) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(f);
  print(mirror.toJSONProtocol(true));
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.ObjectMirror);
  assertTrue(mirror instanceof debug.FunctionMirror);

  // Check the mirror properties.
  assertTrue(mirror.isFunction());
  assertEquals('function', mirror.type());
  assertFalse(mirror.isPrimitive());
  assertEquals("Function", mirror.className());
  assertEquals(f.name, mirror.name());
  assertTrue(mirror.resolved());
  assertEquals(f.toString(), mirror.source());
  assertTrue(mirror.constructorFunction() instanceof debug.ObjectMirror);
  assertTrue(mirror.protoObject() instanceof debug.Mirror);
  assertTrue(mirror.prototypeObject() instanceof debug.Mirror);
  
  // Test text representation
  assertEquals(f.toString(), mirror.toText());

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('function', fromJSON.type);
  assertEquals('Function', fromJSON.className);
  assertEquals('function', fromJSON.constructorFunction.type);
  assertEquals('Function', fromJSON.constructorFunction.name);
  assertTrue(fromJSON.resolved);
  assertEquals(f.name, fromJSON.name);
  assertEquals(f.toString(), fromJSON.source);

  // Check the formatted text (regress 1142074).
  assertEquals(f.toString(), fromJSON.text);
}


// Test a number of different functions.
testFunctionMirror(function(){});
testFunctionMirror(function a(){return 1;});
testFunctionMirror(Math.sin);
