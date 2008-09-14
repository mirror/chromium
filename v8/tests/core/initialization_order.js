// Illustrates the initialization order of various
// variables and functions.

print(typeof function(){});
print(typeof f1);
print(typeof f2);


function f1() {
  print("outer f1");
}


function f2() {
  print(typeof f1);
  f1();
  function f1() {
    print("inner f1");
  }
  f1();
}


f1();
f2();
f1();


function g1() {
  print("outer g1");
}


function g2() {
  print(typeof g1);
  //g1();  // this call should fail because g1 should be undefined!
  var g1 = function() {
    print("inner g1");
  }
  g1();
}


g1();
g2();
g1();


function h1() {
  print("outer h1");
}


function h2() {
  print(typeof h1);
  h1();
  h1 = function() {
    print("inner h1");
  }
  h1();
}


h1();
h2();
h1();
