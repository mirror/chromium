// Make sure that we can create large object literals.
var nofProperties = 150;

// Build large object literal string.
var literal = "var o = { ";

for (var i = 0; i < nofProperties; i++) {
  if (i > 0) literal += ",";
  literal += ("a" + i + ":" + i);
}
literal += "}";


// Create the large object literal
eval(literal);

// Check that the properties have the expected values.
for (var i = 0; i < nofProperties; i++) {
  assertEquals(o["a"+i], i);
}


