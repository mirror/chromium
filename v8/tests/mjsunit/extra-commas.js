function assertSyntaxError(x) {
  var caught = false;
  try {
    eval(x);
  } catch (e) {
    caught = true;
    assertTrue(e instanceof SyntaxError, "is syntax error");
  }
  assertTrue(caught, "throws exception");
};


assertSyntaxError("f(,)");
assertSyntaxError("f(1,)");
assertSyntaxError("f(1,2,)");

assertSyntaxError("function f(,) {}");
assertSyntaxError("function f(1,) {}");
assertSyntaxError("function f(1,2,) {}");
