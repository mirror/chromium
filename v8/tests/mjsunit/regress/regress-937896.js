// This used to crash because the label collector in the parser didn't
// discard duplicates which caused the outer-most continue statement
// to try to unlink the inner try-handler that wasn't on the stack.

function f() {
  try {
    for (var i = 0; i < 2; i++) {
      continue;
      try {
        continue;
        continue;
      } catch (ex) {
        // Empty.
      }
    }
  } catch (e) {
    // Empty. 
  }
  return 42;
}


assertEquals(42, f());
