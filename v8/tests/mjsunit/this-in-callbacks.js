// Test 'this' is the right global object of callback functions passed to
// builtin functions.
// See bug 1231592

var my_identity = 'id';
// test Array.sort
function cp(x, y) {
  assertEquals('id', this.my_identity);
  return 0;
}

[1, 2].sort(cp);

// test String.replace
function r(x) {
  return this.my_identity;
}

assertEquals('id', 'hello'.replace('hello', r));
assertEquals('id', 'hello'.replace(/hello/, r));
