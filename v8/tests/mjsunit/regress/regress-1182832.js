var caught = false;
try {
  (function () {
    var e = 0;
    eval("const e = 1;");
  })();
} catch (e) {
  caught = true;
  assertTrue(e instanceof TypeError);
}
assertTrue(caught);
