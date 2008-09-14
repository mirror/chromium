print("scenario 1");
(function () {
  function f1(x) {

    function f2() {
      print("f2 called");
      return x;
    }

    print("f1 called");
    return f2;
  }

  var t = f1(3);
  print("expected: 3, got: " + t());
})()


print("scenario 2");
(function () {
  function f1(x) {

    function f2() {
      print("f2 called");
      return x;
    }

    print("f1 called");
    var t = f2;
    x = 4;
    return f2;
  }

  var t = f1(3);
  print("expected: 4, got: " + t());
})()


print("scenario 3");
(function () {
  function f2(x, y) {
    print("f2 called");
    var v = 0;
    x(y);
    return v;
  }

  function f1(y) {
    print("f1 called");
    v = y;
  }

  print("expected: 0, got: " + f2(f1, 1));

  // Aliasing of eval is not allowed.  Therefore, this test is
  // disabled.
  //
  // print("expected: 1, got: " + f2(eval, "v = 1;"));
}) ()
