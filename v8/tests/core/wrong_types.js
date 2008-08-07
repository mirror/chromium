// Incorrect types
//
// ecma/ObjectObjects/15.2.4.3.js

var myarray = new Array();
myarray.valueOf = Object.prototype.valueOf;
var myboolean = new Boolean();
myboolean.valueOf = Object.prototype.valueOf;
var myfunction = new Function();
myfunction.valueOf = Object.prototype.valueOf;
var myobject = new Object();
myobject.valueOf = Object.prototype.valueOf;
var mymath = Math;
mymath.valueOf = Object.prototype.valueOf;
var mydate = new Date();
mydate.valueOf = Object.prototype.valueOf;
var mynumber = new Number();
mynumber.valueOf = Object.prototype.valueOf;
var mystring = new String();
mystring.valueOf = Object.prototype.valueOf;

var num_failures = 0
// if (typeof(myarray) != typeof(myarray.valueOf())) {
//   print("Error: new Array() " + typeof(myarray) + " vs " + typeof(myarray.valueOf()))
//   ++num_failures
// }

if (typeof(myboolean) != typeof(myboolean.valueOf())) {
  print("Error: new Boolean() " + typeof(myboolean) + " vs " + typeof(myboolean.valueOf()))
  ++num_failures
}


// if (typeof(myfunction) != typeof(myfunction.valueOf())) {
//   print("Error: new Function() " + typeof(myfunction) + " vs " + typeof(myfunction.valueOf()))
//   ++num_failures
// }


// if (typeof(myobject) != typeof(myobject.valueOf())) {
//   print("Error: new Object() " + typeof(myobject) + " vs " + typeof(myobject.valueOf()))
//   ++num_failures
// }


// if (typeof(mymath) != typeof(mymath.valueOf())) {
//   print("Error: Math " + typeof(mymath) + " vs " + typeof(mymath.valueOf()))
//   ++num_failures
// }


if (typeof(mydate) != typeof(mydate.valueOf())) {
  print("Error: new Date() " + typeof(mydate) + " vs " + typeof(mydate.valueOf()))
  ++num_failures
}


if (typeof(mynumber) != typeof(mynumber.valueOf())) {
  print("Error: new Number() " + typeof(mynumber) + " vs " + typeof(mynumber.valueOf()))
  ++num_failures
}


if (typeof(mystring) != typeof(mystring.valueOf())) {
  print("Error: new String() " + typeof(mystring) + " vs " + typeof(mystring.valueOf()))
  ++num_failures
}



print(num_failures + " failures.")
