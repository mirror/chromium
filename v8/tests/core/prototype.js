function foo() {
}

foo.prototype = { x : 1 }
var a = new foo();
print(a.x)
print(a.y)

foo.prototype = { y : 1 }
var b = new foo();
print(b.y)
print(b.x)


// test __proto__
var c = new Object();
print(c.__proto__ == Object.prototype);
print(b.__proto__ == foo.prototype);

var d = new Function();
print(d.__proto__ == Function.prototype);

// Test that basic objects yield the right prototype.
print((1).__proto__ == Number.prototype);
print("".__proto__ == String.prototype);
print(false.__proto__ == Boolean.prototype);

// Simple tests for assigning to __proto__.
d.__proto__ = null;
print(d.__proto__ == null);
d.__proto__ = foo.prototype;
print(d.y == 1);

// Test assign non JS object values to __proto__
d.__proto__ = 3;
print(d.__proto__ == foo.prototype);
d.__proto__ = undefined;
print(d.__proto__ == foo.prototype);
d.__proto__ = 'foo';
print(d.__proto__ == foo.prototype);

// Check lookup works with a non JS object as prototype. 
function h(){}
h.prototype = 1
print(h.x)

// Check specifying __proto__ in an object literal does not crash the compiler.
// Regression test for bug:
//  824678 ASSERT in v8::internal::Ia32CodeGenerator::VisitObjectLiteral when I log in on www.hotmail.com
print( { __proto__: { } } != new Object());
