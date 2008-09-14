// Exception variables used in try-catch should be scoped, e.g. only
// visible inside the catch block. They should *not* just be treated
// as local variables and they should be allowed to nest.
var e = 0;
try {
  throw e + 1;
} catch(e) {
  try {
    throw e + 1;
  } catch (e) {
    assertEquals(2, e);
  }
  assertEquals(1, e);
}
assertEquals(0, e);
