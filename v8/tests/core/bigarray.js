ARRAY_SIZE = 10000;

start = new Date();

large_array = new Array(ARRAY_SIZE);
for (i=0; i<ARRAY_SIZE; i++)
  large_array[i] = i;

for (i=0; i<ARRAY_SIZE; i++) {
  if (large_array[i] != i) {
    print("fail");
    break;
  }
}

end = new Date();

// print("pass, in "+(end - start) + "ms");
print("Done");
