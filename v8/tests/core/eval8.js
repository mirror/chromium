// Source elements inside eval() calls musy be treated like any other
// source elements: They must be declared upon entering the eval()
// scope.

function f(s) {
  eval(s);
};

f("g(); function g() { print('g'); }");
