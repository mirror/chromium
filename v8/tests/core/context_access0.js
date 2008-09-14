function f0(x) {
  var y = "y";

  function f1() {
    print(x);
    print(y);
  }

  f1();
}

f0("x");


function g0(x) {
  var y = "y";

  function g1() {
    print(x);
    print(y);
  }

  return g1;
}

var g = g0("x");
g();


function h0(x0) {
  var y0 = "y0";

  function h1(x1) {
    var y1 = "y1";

    function h2(x2) {
      var y2 = "y2";

      print("x0 = " + x0);
      print("y0 = " + y0);

      print("x1 = " + x1);
      print("y1 = " + y1);

      print("x2 = " + x2);
      print("y2 = " + y2);

      print("");

      return h2;
    }

    h2("x2");
    return h2;
  }

  h1("x1");
  return h1;
}

var h1 = h0("x0");
var h2 = h1("new x1");

print(typeof h2("new x2"));
