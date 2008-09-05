// Test to make sure that we are seing the 2nd x when printing it
// inside g. This requires that the implementation is copying the
// correct argument into the context object.

function f(x, y, x) {
  function g() {
    print(x);
  }
  g();
}

f(3, 5, 7);  // 7

