// Make sure this doesn't fail with an assertion
// failure during lazy compilation.

var caught = false;
try {
  (function() {
    const a;
    var a;
  })();
} catch (e) {
  caught = true;
}
assertTrue(caught);
