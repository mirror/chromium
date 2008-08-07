// Throw exception when invoking Function.prototype.call with a
// non-function receiver.
var caught = false;
try {
  Function.prototype.call.call({});
  assertTrue(false);
} catch (e) {
  caught = true;
  assertTrue(e instanceof TypeError);
}
assertTrue(caught);
