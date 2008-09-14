var f = Function();
assertTrue(typeof f() == 'undefined');
var f = new Function();
assertTrue(typeof f() == 'undefined');

var f = Function('return 1');
assertEquals(1, f());
var f = new Function('return 1');
assertEquals(1, f());

var f = Function('return true');
assertTrue(f());
var f = new Function('return true');
assertTrue(f());

var f = Function('x', 'return x')
assertEquals(1, f(1));
assertEquals('bar', f('bar'));
assertTrue(typeof f() == 'undefined');
var x = {};
assertTrue(x === f(x));
var f = new Function('x', 'return x')
assertEquals(1, f(1));
assertEquals('bar', f('bar'));
assertTrue(typeof f() == 'undefined');
var x = {};
assertTrue(x === f(x));

var f = Function('x', 'y', 'return x+y');
assertEquals(5, f(2, 3));
assertEquals('foobar', f('foo', 'bar'));
var f = new Function('x', 'y', 'return x+y');
assertEquals(5, f(2, 3));
assertEquals('foobar', f('foo', 'bar'));

var x = {}; x.toString = function() { return 'x'; };
var y = {}; y.toString = function() { return 'y'; };
var z = {}; z.toString = function() { return 'return x*y'; }
var f = Function(x, y, z);
assertEquals(25, f(5, 5));
assertEquals(42, f(2, 21));
var f = new Function(x, y, z);
assertEquals(25, f(5, 5));
assertEquals(42, f(2, 21));

