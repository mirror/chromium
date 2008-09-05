var a = [];
a[4] = 42;

// 3 is not in a, but the following test succeeds.
if (3 in a) {
  print('fail');
} else {
  print ('pass');
}
