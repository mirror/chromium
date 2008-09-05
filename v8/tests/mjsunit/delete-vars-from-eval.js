// Variable declarations in eval() must introduce delete-able vars;
// even when they are local to a function.
(function() {
  eval("var tmp0 = 0");
  assertEquals(0, tmp0);
  assertTrue(delete tmp0);
  assertTrue(typeof(tmp0) == 'undefined');
})();

eval("var tmp1 = 1");
assertEquals(1, tmp1);
assertTrue(delete tmp1);
assertTrue(typeof(tmp1) == 'undefined');
