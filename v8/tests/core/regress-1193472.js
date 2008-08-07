// Fails with an assertion failure in debug mode.

function f() {
  try {
    throw 42;
  } finally {
    return 87;
  }
}

f();

// Used to crash with an assertion failure because
// the externally caught flag was true as a left-over
// from calling f.
print(1234);

 
