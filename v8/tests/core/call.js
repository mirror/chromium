function f() {
  print(this);
}

function g(x) {
  print(this);
  print(x);
}

function h(x, y) {
  print(this);
  print(x);
  print(y);
}


f.call(42);
f.call(42, 87);
f.call(42, 87, 99);

g.call(42);
g.call(42, 87);
g.call(42, 87, 99);

h.call(42);
h.call(42, 87);
h.call(42, 87, 99);
