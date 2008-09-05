// ECMA-252 11.8.7
//
// If the ShiftExpression is not an object, should throw an TypeError.
// Should throw an exception, but not crash VM.

assertThrows("if ('p' in undefined) { }");
assertThrows("if ('p' in null) { }")
assertThrows("if ('p' in true) { }");
assertThrows("if ('p' in 5) { }");
