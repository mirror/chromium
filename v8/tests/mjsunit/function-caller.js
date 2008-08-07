function f(match) {
  g(match);
}

function g(match) {
  assertEquals(f, g.caller);
  assertEquals(match, f.caller);
}

// Check called from function.
function h() {
  f(h);
}
h();

// Check called from top-level.
f(null);

// Check called from eval.
eval('f(eval)');

