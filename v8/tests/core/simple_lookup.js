// This program tests correct variable resolution.

var x = 1;

function f() {
  var result = x;
  var x = 2;
  return result;
}

print(f());  // should be undefined
