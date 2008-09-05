// The following piece of code illustrates that we do not compile
// several versions of functions when they can be shared (have the
// same context).

var g0;
var g10;
var g20;

function f(x) {

  function g() {
    return 1;
  }

  switch (x) {
    case 0:
      g0 = g;
      return;
    case 10:
      g.foo = "foo10";
      g10 = g;
      break;
    case 20:
      g.foo = "foo20";
      g20 = g;
      break;
  }

  f(x - 1);
}


f(100);
print(g10());  // 1
print(g20());  // 1
if (g10 === g20) {
  print(g10.foo == "foo10" && g20.foo == "foo10");  // true
} else {
  print(g10.foo == "foo10" && g20.foo == "foo20");  // true
}
print(typeof g0);
print(typeof g10);
print(typeof g20);
