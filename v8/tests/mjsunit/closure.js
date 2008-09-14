// This test is lifted an old bug (ic_context_bug.js).

function f(n) {
  return function () { return n; }
}

for (var i = 0; i < 10; i++) {
  var a = f(i);
  assertEquals(i, a());
}
