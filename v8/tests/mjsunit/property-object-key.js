var key = { toString: function() { return 'baz'; } }
var object = { baz: 42 };

assertEquals(42, object[key]);
object[key] = 87;
assertEquals(87, object[key]);
object[key]++;
assertEquals(88, object[key]);

