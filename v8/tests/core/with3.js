// An example that illustrates that "function declarations" inside statements
// are not exactly treated like function expressions that are assigned to
// variables (with smjs).

function h() {
  var x = new Object()
  x.f = 2
  x.g = 3;
  with (x) {
    function f() { return 42; }
    var g = function () { return 43; };
    print(f);  // 2
    print(g);  // function () { return 43; }
  }
  print(x.f);  // 2
  print(x.g);  // function () { return 43; }
  print(f);  // function f() { return 42; }
  print(g);  // undefined
}

h();
