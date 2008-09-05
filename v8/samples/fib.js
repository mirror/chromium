/* Sample: Generate and print fibonacci sequence. */

start = new Date();

function fib(n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

print('Fibonacci sequence:');
for (n = 0; n < 30; ++n) {
  f = fib(n)  
  print(f);
}

end = new Date();

print("Done " + (end - start) + "ms");
