var consCalled = false;

function Object() {
  consCalled = true;
}

function Array() {
  consCalled = true;
}

assertFalse(consCalled);
var x1 = { };
assertFalse(consCalled);
var x2 = { a: 3, b: 4 };
assertFalse(consCalled);
var x3 = [ ];
assertFalse(consCalled);
var x4 = [ 1, 2, 3 ];
assertFalse(consCalled);
