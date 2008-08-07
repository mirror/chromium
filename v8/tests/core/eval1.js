// The following simple test cases test the use of contexts.

function check(x, x0) {
  if (x != x0)
    print("got " + x + "; expected " + x0);
}


function f0(s) {
  var x = 0;
  eval(s);
  return x;
}

check(f0("x = 7"), 7);


function f1(s) {
  function g(s) {
    return eval(s);
  }
  var x = 7;
  return g(s);
}

check(f1("x"), 7);


function f2(s) {
  function g(s) {
    eval(s);
  }
  var x = 7;
  g(s);
  return x;
}

check(f2("x = 11"), 11);


// Setting of new variables

eval('global_x = "foobar"')
check(eval("global_x"), "foobar");

function f3(s1, s2) {
  eval(s1);
  return eval(s2);
}

check(f3("x = 991", "x"), 991);


function f4(s1, s2) {
  function g() {
    return eval(s2);
  }
  eval(s1);
  return g();
}

check(f4("x = 007", "x"), 007);
