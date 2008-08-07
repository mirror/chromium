// Test that we get exceptions for invalid left-hand sides.  Also
// tests that if the invalid left-hand side is a function call, the
// exception is delayed until runtime.

// Normal assignments:
assertThrows("12 = 12");
assertThrows("x++ = 12");
assertThrows("eval('var x') = 12");
assertDoesNotThrow("if (false) eval('var x') = 12");

// Pre- and post-fix operations:
assertThrows("12++");
assertThrows("12--");
assertThrows("--12");
assertThrows("++12");
assertThrows("++(eval('12'))");
assertThrows("(eval('12'))++");
assertDoesNotThrow("if (false) ++(eval('12'))");
assertDoesNotThrow("if (false) (eval('12'))++");

// For in:
assertThrows("for (12 in [1]) print(12);");
assertThrows("for (eval('var x') in [1]) print(12);");
assertDoesNotThrow("if (false) for (eval('var x') in [1]) print(12);");

// For:
assertThrows("for (12 = 1;;) print(12);");
assertThrows("for (eval('var x') = 1;;) print(12);");
assertDoesNotThrow("if (false) for (eval('var x') = 1;;) print(12);");

// Assignments to 'this'.
assertThrows("this = 42");
assertThrows("function f() { this = 12; }");
assertThrows("for (this in Array) ;");
assertThrows("for (this = 0;;) ;");
assertThrows("this++");
assertThrows("++this");
assertThrows("this--");
assertThrows("--this");


