// Illustrating the resolution of a variable x.

var x = 0;

function f1() {
  var x = 1;
  function f2() {
    print(x);
  }
  f2();
}

f1();  // should be 1


function f3() {
  function f2() {
    print(x);
  }
  f2();
  var x = 1;
}

f3();  // should be undefined
