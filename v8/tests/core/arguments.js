// The results from these tests don't match the results from
// rhino and smjs - though it's not clear to me (gri) which
// VM is correct. V8 treats the arguments array like specified
// in ECMA 262, 3rd., 10.1.8, p.39, I believe. The other VMs
// treat the arguments array as if it where only readable.

function f0(x, y) {
  x = "x";
  arguments[1] = "y";
  print(arguments.length);
  print(arguments[0]);
  print(y);
  print(arguments[2]);
  print("");
}

f0()
f0(1)
f0(9, 17)
f0(3, 5, 7);  // 3 x y 7
f0("a", "b", "c", "d")  // 4 x y c


function print_args(a) {
  print("typeof a.callee = " + typeof a.callee);
  print("a.length = " + a.length);
  for (i = 0; i < a.length; i++)
    print("a[" + i + "] = " + a[i]);
  print("");
}


function f1(x, y) {
  function g(p) {
    x = p;
  }
  g(y);
  print_args(arguments);
}

f1();
f1(3);
f1(3, 5);
f1(3, 5, 7);


// Testing arguments.callee
print(function() { if (arguments[0] > 0) return arguments.callee(arguments[0] - 1) * arguments[0]; return 1; } (10));
