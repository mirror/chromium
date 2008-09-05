function f0() {
  switch (0) {
  }
}

print("testing f0");
f0();


function f1(x) {
  switch (x) {
    default:
      return "f1";
      break;
  }
  return "foo";
}

print("testing f1");
print(f1(0));
print(f1(1));


function f2(x) {
  var r = 0;
  switch (x) {
    case 0:
      r = "zero";
    case 1:
      r = "one";
      break;
    default:
      return "default";
  }
  return "r = " + r;
}


print("testing f2");
print(f2(0));
print(f2(1));
print(f2(2));


function f3(x, c) {
  var r = 0;
  switch (x) {
    default:
      r = "default";
      break;
    case  c:
      r = "value is c = " + c;
      break;
    case  2:
      r = "two";
      break;
    case -5:
      r = "minus 5";
      break;
    case  9:
      r = "nine";
      break;
  }
  return r;
}


print("testing f3");
print(f3( 2, 0));
print(f3(-5, 0));
print(f3( 9, 0));
print(f3( 0, 0));
print(f3( 2, 2));
print(f3( 7, 0));


function f4(x) {
  switch(x) {
    case 0:
      x++;
    default:
      x++;
    case 2:
      x++;
  }
  return x;
}

print(f4(0));
print(f4(1));
print(f4(2));

