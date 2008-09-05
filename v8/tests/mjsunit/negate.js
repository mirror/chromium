const SMI_MAX = (1 << 30) - 1;
const SMI_MIN = -(1 << 30);

function testmulneg(a, b) {
  var base = a * b;
  assertEquals(-base, a * -b);
  assertEquals(-base, -a * b);
  assertEquals(base, -a * -b);
}

testmulneg(2, 3);
testmulneg(SMI_MAX, 3);
testmulneg(SMI_MIN, 3);
testmulneg(3.2, 2.3);

var x = { valueOf: function() { return 2; } };
var y = { valueOf: function() { return 3; } };

testmulneg(x, y);

// The test below depends on the correct evaluation order, which is not
// implemented by any of the known JS engines.
/*
var z;
var v = { valueOf: function() { z+=2; return z; } };
var w = { valueOf: function() { z+=3; return z; } };

z = 0;
var base = v * w;
z = 0;
assertEquals(-base, v * -w);  // This may fail since we may evaluate "-w" before "v"
z = 0;
assertEquals(-base, -v * w);
z = 0;
assertEquals(base, -v * -w);

*/
