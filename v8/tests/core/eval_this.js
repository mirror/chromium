function T0() {
};

T0.prototype = this;
flubber = function() {
  print("T0: " + this);
};

function T1() {
};
T1.prototype = new T0();

var v = new T1();
v.flubber();


var x = 9;

function foo() {
  eval('print(this.x);');
}
foo(); // pass global in as this

var bar = new Object();
bar.x = 10;
bar.foo = foo;

bar.foo();  // pass bar in as this


function goo(x) {
  return function () { print("context: x = " + x + ", this.x = " + this.x); };
}

var g = goo(11);
this.g();

bar.g = goo(12);
bar.g();
