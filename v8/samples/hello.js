/* Sample: Hello World. */

print('Hello, World!');

print("Here are some numbers for you...");
for (var i = 0; i <= 22; i++)
  print(i + "! = " + (function f(n) { if (n > 0) return f(n-1)*n; else return 1; })(i));
