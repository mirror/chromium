function f0(x) {
  var x = 2;
  return x;
}

print(f0(1));  // should be 2


function f1(x) {
  { var x = 2; }
  return x;
}

print(f1(1));  // should be 2


function f2(x) {
  function g() {
    var x = 2;
  }
  g();
  return x;
}

print(f2(1));  // should be 1


function f3(x, x) {
  return x;
}

print(f3(1, 2));  // should be 2


function f4(x, y, x) {
  return x;
}

print(f4(1, 2));  // should be undefined


function f5(x) {
  function g() {
    x = 2;
  }
  g();
  return x;
}

print(f5(1));  // should be 2


function f6(x) {
  function g() {
    return x;
  }
  return g;
}

print(f6(1)());  // should be 1


function h(f, x) {
  f(x);
}


function f7(x) {
  function g(y) {
    x = y;
  }
  h(g, 3);
  return x;
}

print(f7(1));  // should be 3


function $context() {
};


function f5_(x) {
  function g($c) {
    $c.x = 2;
  }
  var $c = new $context();
  $c.x = x;

  g($c);
  return $c.x;
}

print(f5_(1));  // should be 2
