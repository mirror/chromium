// Must throw TypeError when accessing properties of null.
var caught = false
try {
  null[0];
  assertTrue(false);
} catch (e) {
  caught = true;
  assertTrue(e instanceof TypeError);
}
assertTrue(caught);
