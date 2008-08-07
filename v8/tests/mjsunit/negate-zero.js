function IsNegativeZero(x) {
  assertEquals(0, x);
  var y = 1 / x;
  assertFalse(isFinite(y));
  return y < 0;
}

var pz = 0;
var nz = -0;

assertTrue(IsNegativeZero(nz), "-0");
assertFalse(IsNegativeZero(-nz), "-(-0)");

assertFalse(IsNegativeZero(pz), "0");
assertTrue(IsNegativeZero(-pz), "-(0)");
