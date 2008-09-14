// Return a parameter from inside a with statement.
function f(x) {
  with ({}) {
    return x;
  }
}

assertEquals(5, f(5));


function g(x) {
  function h() {
    with ({}) {
      return x;
    }
  }
  return h();
}

assertEquals(7, g(7));
