/* Tests for Number */

function check(a, b) {
  if (a != b) {
    print("FAILED: the following two objects must be equal.");
    print(a);
    print(b);
  }
}

// Check parsing
print(0.12e3);
print(0.12E3);
print(0.12e+3);
print(0.12e-3);
print(-0.12e3);

check(Number(null), 0)
check(Number(false), 0)
check(Number(true), 1)
// check(Number(undefined), NaN)

// Set max and min smi.
maxSmi =  536870911;
minSmi = -536870912;

// Check partial-smi arithmetic.
var n = 12;
print(n + 2);
print(2 + n);
print(n - 2);
print(2 - n);
print(1 + 1.5);
print(1.5 + 1);
print(1 - 1.5);
print(1.5 - 1);

// Check that double arithmetic works.
print(1.5 + 1.5);
print(1.5 + 2.7);
print(3.1415929 + 2.718281828459045235);
print(maxSmi + maxSmi + maxSmi + maxSmi); 
print(1.5 - 1.5);
print(1.5 - 2.7);
print(3.1415929 - 2.718281828459045235);
print(minSmi + minSmi + minSmi + minSmi);

// Check that binary not works.
print(~ 0);
print(~ 1);
print(~ 2);
print(~ 911);
print(~ -1);
print(~ -2);
print(~ -911);
print(~ minSmi);
print(~ maxSmi);

// Check binary and.
(function () {
  print(0 & 1);
  print(3 & 2);
  print(-1 & 7);
  print(maxSmi & minSmi);
})();
  
// Check binary or.
(function () {
  print(0 | 1);
  print(3 | 2);
  print(-1 | 7);
  print(maxSmi | minSmi);
  print(64.5 | 128.5);
})();

// Check binary xor.
(function () {
  print(0 ^ 1);
  print(3 ^ 2);
  print(-1 ^ 7);
  print(maxSmi ^ minSmi);
})();
  
// Check >> and >>>.
(function () {
  print(8 >> 1);
  print(maxSmi >> 3);
  print(maxSmi >>> 3);
  print(minSmi >> 3);
  print(minSmi >>> 3);
})();

// Check <<.
(function () {
  print(8 << 1);
  print(1 << 31);
  print(maxSmi == (1 << 29) - 1);
  print(minSmi == -(1 << 29));
})();


print((1).toString());
print(3.14 < 7.19);
print(3.14 > 7.19);

// Check *
print(2 * 2);     // smi * smi
print(1.5 * 1.5); // heap numbers * heap number
print(2 * 1.5);   // smi * heap number
print(1.5 * 2);   // heap number * smi 

// Check /
print(2 / 2);     // smi / smi
print(1.5 / 1.5); // heap numbers / heap number
print(2 / 1.5);   // smi / heap number
print(1.5 / 2);   // heap number / smi 

// Check %
print(2 % 2);     // smi % smi
print(1.5 % 1.5); // heap numbers % heap number
print(2 % 1.5);   // smi % heap number
print(1.5 % 2);   // heap number % smi


print(Number("-1e-999"));
