// Test that we can parse the whole file without issuing syntax errors
// and run it until throwing an error at the end.  The exception is
// thrown because message tests must terminate with a non-zero error
// code.

function testIncrementCall() {
  ++fun();
}

print("all parsed");

throw "error";
