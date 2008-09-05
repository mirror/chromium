// Benchmark floating point computation with NaN-s

function run_undef_ops(n) {
  var x, y, z;
  for (var i = 0; i < n; i++) {
    z = x * y + n*z;
  }
  return (z);
}

var start = new Date();
var res = run_undef_ops(5000000);
print("Time (Undef ops): " + (new Date() - start) + " ms.");
print("Result: " + res);
