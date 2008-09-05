var a = new Array();
a.push(1)
a.push(2, 3)
a.push(4, 5, 6)
a.push(7)

for (index = 0; index < 7; index++) {
  print(a[index]);
}

for (index = 0; index < 7; index++) {
  print(a.pop());
}
