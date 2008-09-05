// Flags: --expose-debug-as debug
// Test the mirror object for unresolved functions.

var mirror = new debug.UnresolvedFunctionMirror("f");
var json = mirror.toJSONProtocol(true);

// Check the mirror hierachy for unresolved functions.
assertTrue(mirror instanceof debug.Mirror);
assertTrue(mirror instanceof debug.ValueMirror);
assertTrue(mirror instanceof debug.ObjectMirror);
assertTrue(mirror instanceof debug.FunctionMirror);

// Check the mirror properties for unresolved functions.
assertTrue(mirror.isUnresolvedFunction());
assertEquals('function', mirror.type());
assertFalse(mirror.isPrimitive());
assertEquals("Function", mirror.className());
assertEquals("f", mirror.name());
assertFalse(mirror.resolved());
assertEquals(void 0, mirror.source());
assertEquals('undefined', mirror.constructorFunction().type());
assertEquals('undefined', mirror.protoObject().type());
assertEquals('undefined', mirror.prototypeObject().type());
  
// Parse JSON representation of unresolved functions and check.
/*var fromJSON = eval('(' + json + ')');
assertEquals('function', fromJSON.type);
assertEquals('Function', fromJSON.className);
assertEquals('undefined', fromJSON.constructorFunction.type);
assertEquals('undefined', fromJSON.protoObject.type);
assertEquals('undefined', fromJSON.prototypeObject.type);
assertFalse(fromJSON.resolved);
assertEquals("f", fromJSON.name);
assertEquals(void 0, fromJSON.source);*/
