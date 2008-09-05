// Copyright 2006-2007 Google Inc. All Rights Reserved.
// <<license>>


// Keep reference to original values of some global properties.  This
// has the added benefit that the code in this file is isolated from
// changes to these properties.
const $Infinity = global.Infinity;


// Instance class name can only be set on functions. That is the only
// purpose for MathConstructor.
function MathConstructor() {};
%FunctionSetInstanceClassName(MathConstructor, 'Math');  
const $Math = new MathConstructor();
$Math.__proto__ = global.Object.prototype;
%AddProperty(global, "Math", $Math, DONT_ENUM);


// TODO(1240985): Make this work without arguments to the runtime
// call.
function $Math_random() { return %Math_random(0); }
%AddProperty($Math, "random", $Math_random, DONT_ENUM);

function $Math_abs(x) { return %Math_abs(ToNumber(x)); }
%AddProperty($Math, "abs", $Math_abs, DONT_ENUM);

function $Math_acos(x) { return %Math_acos(ToNumber(x)); }
%AddProperty($Math, "acos", $Math_acos, DONT_ENUM);

function $Math_asin(x) { return %Math_asin(ToNumber(x)); }
%AddProperty($Math, "asin", $Math_asin, DONT_ENUM);

function $Math_atan(x) { return %Math_atan(ToNumber(x)); }
%AddProperty($Math, "atan", $Math_atan, DONT_ENUM);

function $Math_ceil(x) { return %Math_ceil(ToNumber(x)); }
%AddProperty($Math, "ceil", $Math_ceil, DONT_ENUM);

function $Math_cos(x) { return %Math_cos(ToNumber(x)); }
%AddProperty($Math, "cos", $Math_cos, DONT_ENUM);

function $Math_exp(x) { return %Math_exp(ToNumber(x)); }
%AddProperty($Math, "exp", $Math_exp, DONT_ENUM);

function $Math_floor(x) { return %Math_floor(ToNumber(x)); }
%AddProperty($Math, "floor", $Math_floor, DONT_ENUM);

function $Math_log(x) { return %Math_log(ToNumber(x)); }
%AddProperty($Math, "log", $Math_log, DONT_ENUM);

function $Math_round(x) { return %Math_round(ToNumber(x)); }
%AddProperty($Math, "round", $Math_round, DONT_ENUM);

function $Math_sin(x) { return %Math_sin(ToNumber(x)); }
%AddProperty($Math, "sin", $Math_sin, DONT_ENUM);

function $Math_sqrt(x) { return %Math_sqrt(ToNumber(x)); }
%AddProperty($Math, "sqrt", $Math_sqrt, DONT_ENUM);

function $Math_tan(x) { return %Math_tan(ToNumber(x)); }
%AddProperty($Math, "tan", $Math_tan, DONT_ENUM);

function $Math_atan2(x, y) { return %Math_atan2(ToNumber(x), ToNumber(y)); }
%AddProperty($Math, "atan2", $Math_atan2, DONT_ENUM);

function $Math_pow(x, y) { return %Math_pow(ToNumber(x), ToNumber(y)); }
%AddProperty($Math, "pow", $Math_pow, DONT_ENUM);

function $Math_max(arg1, arg2) {  // length == 2
  var r = -$Infinity;
  for (var i = %_ArgumentsLength() - 1; i >= 0; --i) {
    var n = ToNumber(%_Arguments(i));
    if (%NumberIsNaN(n)) return n;
    // Make sure +0 is consider greater than -0.
    if (n > r || (n === 0 && r === 0 && (1 / n) > (1 / r))) r = n;
  }
  return r;
}
%AddProperty($Math, "max", $Math_max, DONT_ENUM);

function $Math_min(arg1, arg2) {  // length == 2
  var r = $Infinity;
  for (var i = %_ArgumentsLength() - 1; i >= 0; --i) {
    var n = ToNumber(%_Arguments(i));
    if (%NumberIsNaN(n)) return n;
    // Make sure -0 is consider less than +0.
    if (n < r || (n === 0 && r === 0 && (1 / n) < (1 / r))) r = n;
  }
  return r;
}
%AddProperty($Math, "min", $Math_min, DONT_ENUM);


// ECMA-262, section 15.8.1.1.
%AddProperty($Math, "E", 2.7182818284590452354, DONT_ENUM |  DONT_DELETE | READ_ONLY);

// ECMA-262, section 15.8.1.2.
%AddProperty($Math, "LN10", 2.302585092994046, DONT_ENUM |  DONT_DELETE | READ_ONLY);

// ECMA-262, section 15.8.1.3.
%AddProperty($Math, "LN2", 0.6931471805599453, DONT_ENUM |  DONT_DELETE | READ_ONLY);

// ECMA-262, section 15.8.1.4.
%AddProperty($Math, "LOG2E", 1.4426950408889634, DONT_ENUM |  DONT_DELETE | READ_ONLY);
%AddProperty($Math, "LOG10E", 0.43429448190325176, DONT_ENUM |  DONT_DELETE | READ_ONLY);
%AddProperty($Math, "PI", 3.1415926535897932, DONT_ENUM |  DONT_DELETE | READ_ONLY);
%AddProperty($Math, "SQRT1_2", 0.7071067811865476, DONT_ENUM |  DONT_DELETE | READ_ONLY);
%AddProperty($Math, "SQRT2", 1.4142135623730951, DONT_ENUM |  DONT_DELETE | READ_ONLY);
