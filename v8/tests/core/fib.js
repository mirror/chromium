function IterativeFib(n) {
  var f0 = 0, f1 = 1;
  for (; n > 0; --n) {
    var f2 = f0 + f1;
    f0 = f1; f1 = f2;
  }
  return f0;
}

function RecursiveFib(n) {
  if (n <= 1) return n;
  return RecursiveFib(n - 1) + RecursiveFib(n - 2);
}

function Check(n, expected) {
  var i = IterativeFib(n);
  var r = RecursiveFib(n);
  if (i != expected) { print("Iterative failed:"); print(i); return; }
  if (r != expected) { print("Recursive failed:"); print(r); return; }
  print(expected);
}

Check(0, 0);
Check(1, 1);
Check(2, 1);
Check(3, 1 + 1);
Check(4, 2 + 1);
Check(5, 3 + 2);
Check(10, 55);
Check(15, 610);
Check(20, 6765);

print(IterativeFib(75));

