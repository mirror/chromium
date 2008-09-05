function f() {
  // Force eager compilation of x through the use of eval. The break
  // in function x should not try to break out of the enclosing while.
  return eval("while(0) function x() { break; }; 42");
};

assertEquals(42, f());

