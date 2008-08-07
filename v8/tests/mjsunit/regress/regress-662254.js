function f() {
  for (var c in []) { }
}

f();


function g() {
  var c;
  for (c in []) { }
}

g();
