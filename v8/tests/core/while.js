print("while (false)");
while (false)
  print("should never reach here");

print("while (true)");
var i = 10;
while (true) {
  print(i);
  i--;
  if (i == 0)
    break;
}

print("while w/ continue");
i = 10;
while (i-- > 0)
  continue;

print("while w/ break");
while (true) {
  print("iteration");
  break;
}

print("while while w/ break");
foo: while (true) {
  print("iteration A");
  bar: while (true) {
    print("iteration B");
    break foo;
  }
}

print("while w/ continue & break");
i = 0;
loop: while (i < 10) {
  i++;
  switch (i) {
    case 5: continue;
    case 8: break loop;
  }
  print(i);
}
