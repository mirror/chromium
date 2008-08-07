// ecma//Expressions/11.2.1-1
// ecma/FunctionObjects/15.3.4.1.js
// ecma/FunctionObjects/15.3.5-2.js
//
// Missing system properties

var num_failures = 0;

if (typeof(Function.prototype.length) == "undefined") {
  print("ERROR: Function.prototype.length is undefined")
  ++num_failures
}

if (typeof(Object.constructor) == "undefined") {
  print("ERROR: Object.constructor is undefined")
  ++num_failures
}

if (typeof(String.prototype.constructor) == "undefined") {
  print("ERROR: String.prototype.constructor is undefined")
  ++num_failures
}

if (typeof(String.prototype.length) == "undefined") {
  print("ERROR: String.prototype.length is undefined")
  ++num_failures
}

if (typeof(Boolean.constructor) == "undefined") {
  print("ERROR: Boolean.constructor is undefined")
  ++num_failures
}

if (typeof(Number.prototype.constructor) == "undefined") {
  print("ERROR: Number.prototype.constructor is undefined")
  ++num_failures
}

if (typeof(Function.prototype.constructor) == "undefined") {
  print("ERROR: Function.prototype.constructor is undefined")
  ++num_failures
}

var MyObject = new Function('a', 'this.a = a')
if (typeof(MyObject.prototype.constructor) == "undefined") {
  print("ERROR: MyObject.prototype.constructor is undefined")
  ++num_failures
}


print(num_failures + " failures.")
