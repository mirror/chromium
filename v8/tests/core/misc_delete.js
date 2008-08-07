// Some basic delete tests.

try {
  x = 0;
  delete x;
  print(x);
} catch (e) {
  print(e);
}

try {
  with ( { x: 0 }) {
    delete x;
    print(x);
  }
} catch (e) {
  print(e);
}


print(f);
with ({ f: 0 }) {
  print(f);
  with (0) {
    print(f);
    var f = 3;
    print(f);
  }
  print(f);
}
print(f);  // should be undefined
