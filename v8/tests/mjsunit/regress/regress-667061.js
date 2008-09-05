// Test non-ICC case.
var caught = false;
try {
  (('foo'))();
} catch (o) {
  assertTrue(o instanceof TypeError);
  caught = true;
}
assertTrue(caught);


// Test uninitialized case.
function h(o) {
  return o.x();
}

var caught = false;
try {
  h({ x: 1 });
} catch (o) {
  assertTrue(o instanceof TypeError);
  caught = true;
}
assertTrue(caught);


// Test monomorphic case.
function g(o) {
  return o.x();
}

function O(x) { this.x = x; };
var o = new O(function() { return 1; });
assertEquals(1, g(o));  // go monomorphic
assertEquals(1, g(o));  // stay monomorphic

var caught = false;
try {
  g(new O(3));
} catch (o) {
  assertTrue(o instanceof TypeError);
  caught = true;
}
assertTrue(caught);


// Test megamorphic case.
function f(o) {
  return o.x();
}

assertEquals(1, f({ x: function () { return 1; }}));  // go monomorphic
assertEquals(2, f({ x: function () { return 2; }}));  // go megamorphic
assertEquals(3, f({ x: function () { return 3; }}));  // stay megamorphic

var caught = false;
try {
  f({ x: 4 });
} catch (o) {
  assertTrue(o instanceof TypeError);
  caught = true;
}
assertTrue(caught);
