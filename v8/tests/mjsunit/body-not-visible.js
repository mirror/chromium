// Make sure we cannot see the local variables in NewFunction when
// compiling functions using new Function().

var caught = false;
try {
  (new Function("return body;"))();
  assertTrue(false);
} catch (e) {
  caught = true;
  assertTrue(e instanceof ReferenceError);
}
assertTrue(caught);
