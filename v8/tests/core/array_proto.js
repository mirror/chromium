// ecma/Array/15.4.4.js
//
// Array.prototype.length must be 0
//
// ecma/Array/15.4.3.1-2.js 
//
// Array prototype is not setable or deletable (same goes for Boolean.prototype)
//
// ecma/FunctionObjects/15.3.3.1-4.js

var num_failures = 0
if (Array.prototype.length != 0) {
  print("\nArray.prototype.length should be 0, but got " + Array.prototype.length);
  ++num_failures
}
var before_p = Array.prototype
Array.prototype = null
var after_p = Array.prototype
if (after_p != before_p) {
  print("\nArray.prototype not settable; BEFORE and AFTER should be the same")
  print('BEFORE - Array.prototype: "' + before_p + '"')
  print('AFTER - Array.prototype: "' + after_p + '"\n')
  ++num_failures
}

before_p = Function.prototype
Function.prototype = null
after_p = Function.prototype
if (after_p != before_p) {
  print("\nFunction.prototype not settable; BEFORE and AFTER should be the same")
  print('BEFORE - Function.prototype: "' + before_p + '"')
  print('AFTER - Function.prototype: "' + after_p + '"\n')
  ++num_failures
}

print(num_failures + " failures.")