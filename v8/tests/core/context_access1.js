// Implementation of an array using closures.

function array(n) {
  var element, tail;

  function m(op, i, x) {
    switch (op) {
      case "length":
        return n;
      case "get":
        if (i == 0) return element;
        else return tail(op, i - 1);
      case "set":
        if (i == 0) element = x;
        else tail(op, i - 1, x);
        return;
    }
  }

  if (n > 0) {
    tail = array(n - 1);
    return m;
  }

  return null;
}


function check(x, y) {
  if (x != y)
    print("x = " + x + ", y = " + y);
}


var a = array(10);


for (i = 0; i < a("length"); i++)
  a("set", i, i);


for (i = 0; i < a("length"); i++)
  check(a("get", i), i);
