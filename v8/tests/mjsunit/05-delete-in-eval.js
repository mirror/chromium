// Should be able to delete properties in the context through eval().
tmp = 0;
assertTrue(eval("delete XXX"));  // non-existing
assertTrue(eval("delete tmp"));  // existing
assertFalse("tmp" in this);

// This is similar to 02, I think.