// Math read-only and don-enum properties

// ecma/Math/15.8.1.1-1.js
// ...
// ecma/Math/15.8.1.8-1.js

var num_failures = 0
var old_math = Math.E
Math.E = 0
if (old_math != Math.E) {
  print("ERROR: Math.E should be read-only!")
  ++num_failures
}

old_math = Math.LN10
Math.LN10 = 0
if (old_math != Math.LN10) {
  print("ERROR: Math.LN10 should be read-only!")
  ++num_failures
}

old_math = Math.LN2
Math.LN2 = 0
if (old_math != Math.LN2) {
  print("ERROR: Math.LN2 should be read-only!")
  ++num_failures
}

old_math = Math.LOG2E
Math.LOG2E = 0
if (old_math != Math.LOG2E) {
  print("ERROR: Math.LOG2E should be read-only!")
  ++num_failures
}

// and so on ...
// Math.LOG10E = 0.4342944819032518;
// Math.PI = 3.1415926535897932;
// Math.SQRT1_2 = 0.7071067811865476;
// Math.SQRT2 = 1.4142135623730951;

print(num_failures + " failures.")


var MATHPROPS='';for( p in Math ){ MATHPROPS += p + ","; };
if (MATHPROPS != '') {
  print("ERROR: Math properties are DontEnum; got " + MATHPROPS)
}

