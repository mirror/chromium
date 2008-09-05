function sum(a, b, c) {
  var result = 0;
  for (var i = 0; i < arguments.length; i++) {
    result += arguments[i];
  }
  return result;
}

// Try invoking call before sum has been compiled lazily.
assertEquals(6, sum.call(this, 1, 2, 3), "lazy call");

assertEquals(6, sum(1, 2, 3), "normal");
assertEquals(6, sum.call(this, 1, 2, 3), "call");
assertEquals(6, sum.apply(this, [1, 2, 3]), "apply");
