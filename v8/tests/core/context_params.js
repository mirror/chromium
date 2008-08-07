function f(x, y, x) {

  function g() {
    return x;
  }

  return g();
}

print(f(2, 3, 5));


function h(x, y, x) {
  x = 0;
  print(arguments[0]);
  print(arguments[1]);
  print(arguments[2]);
}

h(2, 3, 5);
