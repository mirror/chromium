function f() { return this; }

assertFalse(this == null);  // the global object shouldn't be null or undefined
assertEquals('[object global]', String(this));

assertTrue(this === this);
assertTrue(this === (function() { return this; })());
assertTrue(this === f());

var x = {}, y = {};
x.f = y.f = f; 
assertFalse(x === f());
assertFalse(y === f());
assertTrue(x === x.f());
assertTrue(x === x[new String('f')]());
assertTrue(y === y.f(), "y.f()");
assertTrue(y === y[new String('f')]());
assertFalse(x === y.f());
assertFalse(y === x.f());
