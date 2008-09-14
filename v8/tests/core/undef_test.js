// 3/9/07 srdjan@google.com
// Bug in handling of undefined variables; V8 does not throw exception when 
// accessing undefined variables.
//
// ecma/LexicalConventions/7.2-2-n.js
// ecma/LexicalConventions/7.2-3-n.js
// ecma/LexicalConventions/7.2-4-n.js
// ecma/LexicalConventions/7.2-5-n.js

try {
  eval("\r\r\r\nb")
  print("Error, BAD evaluation. Should have thrown exception for undefined variable.")
} catch(e) {
  print("CORRECT, exception thrown: " + e)
}


// print('\n**Test should print: ReferenceError: "a" is not defined')
// print('**Test should throw exception: ReferenceError: "b" is not defined\n')
try {
  b=a+2  // Should throw exception
  print(b)
  print("Error, an exception should have been thrown.")
} catch(e) {
  print("CORRECT, exception thrown: " + e)
}

