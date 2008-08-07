function f() {
  try {
    return 42;
  } finally {
    var executed = false;
    while (!executed) {
      try {
        break;
      } finally {
        executed = true;
      }
      assertTrue(false, "should not reach here");
    }
    assertTrue(executed, "finally block executed");
  }
  return 87;
};

assertEquals(42, f());
