// To be compatible with KJS syntax errors for illegal return, break
// and continue should be delayed to runtime.

// Do not throw syntax errors for illegal return, break and continue
// at compile time.
assertDoesNotThrow("if (false) return;");
assertDoesNotThrow("if (false) break;");
assertDoesNotThrow("if (false) continue;");

// Throw syntax errors for illegal return, break and continue at
// compile time.
assertThrows("return;");
assertThrows("break;");
assertThrows("continue;");
