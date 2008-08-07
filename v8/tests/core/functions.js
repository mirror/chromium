var f = function (x) { print(x); };
f(42);
f('Hello');

var g = function foo(x) { print(x); };
g(87);
g('World');

// Check the common optional argument idiom.
function foo(x, y, optZ) {
  z = optZ || 'default z';
  print(x + ':' + y + ':' + z);
}

foo('bar');
foo('bar', 'baz');
foo('bar', 'baz', 'buz');


var o = [];
o.x = function () { return 'fail'; };
print(o.x(o.x = function () { return 'okay'; }));

/*
var a = [];
a[4] = function () { return 'fail'; };
print(a[4](a[4] = function () { return 'okay'; }));
*/

print(typeof foo == 'function');
