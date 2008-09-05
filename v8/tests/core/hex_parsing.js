// Tests corned cases of hexadecimal constants
// (too large to accurately represent in a double).

var num_failures = 0
var k = 0x1000000000000081
if (k != 1152921504606847200) {
  print("Error: 0x1000000000000081 is 1152921504606847200, but got " + k)
  ++num_failures
}

k = 0x1000000000000281
if (k != 1152921504606847700) {
  print("Error: 0x1000000000000281 is 1152921504606847700, but got " + k)
  ++num_failures
}

k = 0x10000000000002810
if (k != 18446744073709564000) {
  print("Error: 0x10000000000002810 is 18446744073709564000, but got " + k)
  ++num_failures
}

k = 0x10000000000002810000
if (k != 7.555786372591437e+22) {
  print("Error: 0x10000000000002810000 is 7.555786372591437e+22, but got " + k)
  ++num_failures
}

k = 0xffffffffffffffff
if (k != 18446744073709552000) {
  print("Error: 0xffffffffffffffff is 18446744073709552000, but got " + k)
  ++num_failures
}

k = 0xffffffffffffffffffff
if (k != 1.2089258196146292e+24) {
  print("Error: 0xffffffffffffffffffff is 1.2089258196146292e+24, but got " + k)
  ++num_failures
}

print(num_failures + " failures.")
