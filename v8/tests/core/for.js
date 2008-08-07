print("simple for loop");
for (var i = 0; i < 10; i++)
  print(i);

print("simple for loop w/ break");
for (var i = 0; i < 10; i++) {
  if (i == 7)
    break;
  print(i);
}

print("simple for loop w/ break & continue");
for (var i = 0; i < 10; i++) {
  if (i == 3)
    continue;
  if (i == 7)
    break;
  print(i);
}

print("simple for loop w/ break & continue, using a nested switch");
loop: for (var i = 0; i < 10; i++) {
  switch (i) {
    case 3: continue;
    case 7: break loop;
  }
  print(i);
}

print("endless for loop");
for (var i = 0; ; i--) {
  print(i);
  if (i < -5)
    break;
}
