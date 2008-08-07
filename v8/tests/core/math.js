function check(a, b) {
  if (a != b) {
    print("FAILED: two values must be equal");
    print(a);
    print(b);
  }
}

function verify(c, msg) {
  if (!c) {
    print("FAILED: "+msg);
  }
}

check(Math.E, 2.7182818284590452354);
check(Math.LN10, 2.302585092994046);
check(Math.LN2, 0.6931471805599453);
check(Math.LOG2E, 1.4426950408889634);
check(Math.LOG10E, 0.43429448190325176);
check(Math.PI, 3.1415926535897932);
check(Math.SQRT1_2, 0.7071067811865476);
check(Math.SQRT2, 1.4142135623730951);

// test Math.abs
check(Math.abs(-1.02), 1.02);
check(Math.abs(1.02), 1.02);

verify(isNaN(Math.abs(NaN)), "Math.abs(NaN) is not NaN");
check(Math.abs(-0), 0);
verify(!isFinite(Math.abs(Infinity)), "Math.abs(-Infinity) is not infinity");
verify(!isFinite(Math.abs(-Infinity)), "Math.abs(-Infinity) is not infinity");

// test Math.acos
verify(isNaN(Math.acos(NaN)), "Math.acos(NaN) is not NaN");
verify(isNaN(Math.acos(1.000001)), "Math.acos(1.00001) is not NaN");
verify(isNaN(Math.acos(-1.000001)), "Math.acos(-1.00001) is not NaN");
check(Math.acos(1), 0);

// test Math.floor
check(Math.floor(3.01), 3);


print("Pass");
