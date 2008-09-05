// Regression test for issue 875031.

var caught = false;
try {
  eval("return;");
  assertTrue(false);  // should not reach here
} catch (e) {
  caught = true;
}
assertTrue(caught);
