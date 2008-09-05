// Flags: --expose-debug-as debug
// Test the mirror object for objects

function testArrayMirror(a, names) {
  // Create mirror and JSON representation.
  var mirror = debug.MakeMirror(a);
  var json = mirror.toJSONProtocol(true);

  // Check the mirror hierachy.
  assertTrue(mirror instanceof debug.Mirror);
  assertTrue(mirror instanceof debug.ValueMirror);
  assertTrue(mirror instanceof debug.ObjectMirror);
  assertTrue(mirror instanceof debug.ArrayMirror);

  // Check the mirror properties.
  assertTrue(mirror.isArray());
  assertEquals('object', mirror.type());
  assertFalse(mirror.isPrimitive());
  assertEquals('Array', mirror.className());
  assertTrue(mirror.constructorFunction() instanceof debug.ObjectMirror);
  assertTrue(mirror.protoObject() instanceof debug.Mirror);
  assertTrue(mirror.prototypeObject() instanceof debug.Mirror);
  assertEquals(mirror.length(), a.length);
  
  var indexedValueMirrors = mirror.indexedPropertiesFromRange();
  assertEquals(indexedValueMirrors.length, a.length);
  for (var i = 0; i < indexedValueMirrors.length; i++) {
    assertTrue(indexedValueMirrors[i] instanceof debug.Mirror);
    if (a[i]) {
      assertTrue(indexedValueMirrors[i] instanceof debug.PropertyMirror);
    }
  }

  // Parse JSON representation and check.
  var fromJSON = eval('(' + json + ')');
  assertEquals('object', fromJSON.type);
  assertEquals('Array', fromJSON.className);
  assertEquals('function', fromJSON.constructorFunction.type);
  assertEquals('Array', fromJSON.constructorFunction.name);
  assertEquals(a.length, fromJSON.length);

  // Check that the serialization contains all indexed properties.
  for (var i = 0; i < fromJSON.indexedProperties.length; i++) {
    var index = fromJSON.indexedProperties[i].name;
    assertEquals(indexedValueMirrors[index].name(), index);
    assertEquals(indexedValueMirrors[index].value().type(), fromJSON.indexedProperties[i].value.type, index);
  }

  // Check that the serialization contains all names properties.
  if (names) {
    for (var i = 0; i < names.length; i++) {
      var found = false;
      for (var j = 0; j < fromJSON.properties.length; j++) {
        if (names[i] == fromJSON.properties[j].name) {
          found = true; 
        }
      }
      assertTrue(found, names[i])
    }
  } else {
    assertEquals(1, fromJSON.properties.length)
  }
}


// Test a number of different arrays.
testArrayMirror([]);
testArrayMirror([1]);
testArrayMirror([1,2]);
testArrayMirror(["a", function(){}, [1,2], 2, /[ab]/]);

a=[1];
a[100]=7;
testArrayMirror(a);

a=[1,2,3];
a.x=2.2;
a.y=function(){return null;}
testArrayMirror(a, ['x','y']);

var a = []; a.push(a);
testArrayMirror(a);
