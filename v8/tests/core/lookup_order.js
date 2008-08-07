// Some issues w/ eval and changing inner functions.

function f0() {
  print("outer f0");
}


function f1(s) {
  f0();
  function f0() {
    print("inner f0");
  }
  eval(s);
  f0();
}


f1(";");
f1("function f0() { print(\"injected f0\"); }");
f1("var f0 = function() { print(\"injected f0\"); }");


function g0() {
  print("outer g0");
}


function g1(s) {
  g0();
  g0 = function() {
    print("inner g0");
  }
  eval(s);
  g0();
}


g1(";");
g1("function g0() { print(\"injected g0\"); }");
g1("var g0 = function() { print(\"injected g0\"); }");
