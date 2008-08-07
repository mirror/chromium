function A() { }
function B() { }
function C() { }

function NewC() {
  A.prototype = {};
  B.prototype = new A();
  C.prototype = new B();
  var result = new C();
  result.A = A.prototype;
  result.B = B.prototype;
  result.C = C.prototype;
  return result;
}

// Check that we can read properties defined in prototypes.
var c = NewC();
c.A.x = 1;
c.B.y = 2;
c.C.z = 3;
assertEquals(1, c.x);
assertEquals(2, c.y);
assertEquals(3, c.z);

var c = NewC();
c.A.x = 0;
for (var i = 0; i < 2; i++) {
  assertEquals(i, c.x);
  c.B.x = 1;
}


// Regression test:
// Make sure we preserve the prototype of an object in the face of map transitions.

function D() {
  this.d = 1;
}
var p = new Object();
p.y = 1;
new D();

D.prototype = p
assertEquals(1, (new D).y);


// Regression test:
// Make sure that arrays and functions in the prototype chain works;
// check length.
function X() { }
function Y() { }

X.prototype = function(a,b) { };
Y.prototype = [1,2,3];

assertEquals(2, (new X).length);
assertEquals(3, (new Y).length);


// Test setting the length of an object where the prototype is from an array.
var test = new Object;
test.__proto__ = (new Array()).__proto__;
test.length = 14;
assertEquals(14, test.length);


