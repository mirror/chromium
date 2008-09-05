// Flags: --expose-debug-as debug
// Test the mirror object for undefined

// Create mirror and JSON representation.
var mirror = debug.MakeMirror(void 0);
var json = mirror.toJSONProtocol(true);

// Check the mirror hierachy.
assertTrue(mirror instanceof debug.Mirror);
assertTrue(mirror instanceof debug.UndefinedMirror);

// Check the mirror properties.
assertTrue(mirror.isUndefined());
assertEquals('undefined', mirror.type());
assertTrue(mirror.isPrimitive());

// Test text representation
assertEquals('undefined', mirror.toText());

// Parse JSON representation and check.
var fromJSON = eval('(' + json + ')');
assertEquals('undefined', fromJSON.type);