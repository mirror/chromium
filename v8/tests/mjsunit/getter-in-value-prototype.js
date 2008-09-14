// Test that getters can be defined and called on value prototypes.
//
// This used to fail because of an invalid cast of the receiver to a
// JSObject.

String.prototype.__defineGetter__('x', function() { return this; });
assertEquals('asdf', 'asdf'.x);

