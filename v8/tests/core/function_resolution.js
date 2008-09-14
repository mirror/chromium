function f1() {
  print("outer f1");
}


function f0() {
  f1();
  f1 = function() {
    print("assigned f1");
  }
  f1();
  function f1() {
    print("declared f1");
  }
  f1();
}


f0();


function f2() {
  function g0() {
    print("g0");
  }
  var g0;  // variable declaration must leave g0 untouched
  g0();
}


f2();
