// This test checks proper assignment/initialization of functions.

var f0;
var f1 = function n1(x) {
  f0 = n1;
  if (x > 0) return n1(x - 1) + x;
  return 0;
};

print(f1(10));
print(f0(10));

g1();

var g1 = function g1() {
  print("g1 (1)");
}

function g1() {
  print("g1 (2)");
}

g1();
