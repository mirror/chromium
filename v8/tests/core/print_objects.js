// ecma//Expressions/11.1.1.js
//
// Test printing of Objects; currently two failures, two successes

function MyObject() {
}


var num_failures = 0

if (this.toString() != "[object global]") {
  print("ERROR: 'this' should be '[object global]', but got '" + this.toString() + "'")
  ++num_failures;
}

var obj1 = { f1: "a", f2: "b"}
if (obj1.toString() != "[object Object]") {
  print("ERROR: 'obj1' should be '[object Object]', but got '" + obj1.toString() + "'")
  ++num_failures;
}

var obj2 = new Object()
if (obj2.toString() != "[object Object]") {
  print("ERROR: 'obj2' should be '[object Object]', but got '" + obj2.toString() + "'")
  ++num_failures;
}

var obj3 = new MyObject()
if (obj3.toString() != "[object Object]") {
  print("ERROR: 'obj3' should be '[object Object]', but got '" + obj3.toString() + "'")
  ++num_failures;
}

print(num_failures + " failures.")
