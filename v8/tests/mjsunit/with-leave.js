L: with ({x:12}) {
  assertEquals(12, x);
  break L;
  assertTrue(false);
}

do {
  with ({x:15}) {
    assertEquals(15, x);
    continue;
    assertTrue(false);
  }
} while (false);

var caught = false;
try {
  with ({x:18}) { throw 25; assertTrue(false); }
} catch (e) {
  caught = true;
  assertEquals(25, e);
  with ({y:19}) {
    assertEquals(19, y);
    try {
      // NOTE: This checks that the object containing x has been
      // removed from the context chain.
      x;
      assertTrue(false);  // should not reach here
    } catch (e2) {
      assertTrue(e2 instanceof ReferenceError);
    }
  }
}
assertTrue(caught);

