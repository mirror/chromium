// Flags: --expose-debug-as debug
// Test the mirror object for null

// Create mirror and JSON representation.
var mirror = debug.MakeMirror(null);
var json = mirror.toJSONProtocol(true);

// Check the mirror hierachy.
assertTrue(mirror instanceof debug.Mirror);
assertTrue(mirror instanceof debug.NullMirror);

// Check the mirror properties.
assertTrue(mirror.isNull());
assertEquals('null', mirror.type());
assertTrue(mirror.isPrimitive());

// Test text representation
assertEquals('null', mirror.toText());

// Parse JSON representation and check.
var fromJSON = eval('(' + json + ')');
assertEquals('null', fromJSON.type);
