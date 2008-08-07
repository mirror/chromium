// ecma/ExecutionContexts/10.2.3-2.js
//
// 'delete' operator is for removing properties from objects and elements
// from arrays. It should not delete objects themselves.
//
// Additonal rules for 'delete':
// ecma/Array/15.4.3.1-2.js
// ecma/Array/15.4.4.4-1.js
// ecma/Array/15.4.4.4-2.js
// ecma/Array/15.4.5.2-1.js
//
// ecma/Boolean/15.6.3.1-2.js
//
// ecma/FunctionObjects/15.3.3.1-3.js
//
// ecma/GlobalObject/15.1.2.2-1.js
// ecma/GlobalObject/15.1.2.3-1.js
// ecma/GlobalObject/15.1.2.3-1.js
// ecma/GlobalObject/15.1.2.5-1.js
//
// 'delete' operator should not delete Array.prototype
var a = "Hi"
var a2 = a
var res = delete a
if (res) {
  print('\n**"delete a" returned "true" instead of "false".')
}
if (a != a2) {
  print('**"delete a" should not have affected "a".')
}

var res2 = delete Array.prototype
if (res2) {
  print('**"delete Array.prototype" should return false.')
}


var res3 = delete Array.prototype.reverse.length 
if (res3) {
  print('**"delete Array.prototype.reverse.length" should return false.')
}

var arr = new Array()
var res4 = delete arr.length
if (res4) {
  print('***"delete Array.length" should return false.')
}

var res5 = delete Boolean.prototype
if (res5) {
  print('***"delete Boolean.prototype" should return false.')
}

var res6 = delete Function.prototype
if (res6) {
  print('***"delete Function.prototype" should return false.')
}

var res7 = delete escape.length
if (res7) {
  print('***"delete escape.length" should return false.')
}

var res8 = delete unescape.length
if (res8) {
  print('***"delete unescape.length" should return false.')
}

// Even rhino and smjs fail this test ?!?
// var res9 = delete isFinite.length
// if (isFinite) {
//   print('***"delete isFinite.length" should return false.')
// }

if (!(delete "Hello")) {
  print('***"delete "Hello"" should return true (default).')
}

function MyFunc(x) {
  return delete x;
}

if (MyFunc("Hello")) {
  print('***"delete"  of argument inside function should return false .')
}

