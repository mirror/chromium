// Tests to verify proper arguments handling if the arguments
// variable is declared as a parameter or local variable.

function e(a) {
  print(a.length);
  print(a);
  print("");
};

e("arguments");


function f(arguments) {
  print(arguments.length);
  print(arguments);
  print("");
};

f("arguments");


function g(x) {
  var arguments;
  print(x);
  print(arguments.length);
  print(arguments);
  print("");
};

g("arguments");


function h(x) {
  print(x);
  print(arguments.length);
  print(arguments);
  var arguments = "foobar";
  print(x);
  print(arguments.length);
  print(arguments);
  print("");
};

h("arguments");
