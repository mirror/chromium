var i;

print("case 1");
i = 10;
while (i-- > 0) {
  print(i);
  if (i == 5)
    ;
}

print("case 2");
i = 10;
while (i-- > 0) {
  print("i = " + i);
  if (i == 5)
    break;
}

print("case 3");
i = 10;
outer: while (i-- > 0) {
  var j = 10;
  inner1: inner2: inner3: while (j-- > 0) {
    print("i = " + i + ", j = " + j);
    if (i == 8)
      break inner2;
    if (i == 6)
      break outer;
  }
}


print("case 4");
outer2: {
  print("1");
  break outer2;
  print("2");
}
print("3");

print("case 5");
print("1");
outer3: break outer3;  // nop 
l1: l2: l3: break l2; // nop
print("2");