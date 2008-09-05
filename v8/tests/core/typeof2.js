// Tests to verify that typeof does not throw reference errors
// under various conditions (e.g. variable in context, conditional
// expression, etc.)

function f0() {
  print("f0: typeof foo = " + typeof foo);
}


function f1(s) {
  eval(s);  // force a context creation
  print("f1: typeof foo = " + typeof foo);
}


function F() {
  this.foo = 0;
  return this;
}


function f2(s) {
  eval(s);  // force a context creation
  var obj = new F();
  print("f2: typeof obj.foo = " + typeof obj.foo);
  print("f2: typeof obj.bar = " + typeof obj.bar);
}


function f3(s) {
  eval(s);  // force a context creation
  var sel = true;
  var foo = "foo";
  print("f3: typeof ( sel ? foo : bar) = " + typeof ( sel ? foo : bar));
  print("f3: typeof (!sel ? foo : bar) = " + typeof (!sel ? foo : bar));
}


f0();
f1();
f2();
f3();
