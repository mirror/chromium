function catcher(o, p) {
  try { o[p]; } catch (e) { return e; }
  throw p;
}

assertTrue(catcher(null, 'foo') instanceof TypeError);
assertTrue(catcher(void 0, 'foo') instanceof TypeError);
assertTrue(catcher(null, 123) instanceof TypeError);
assertTrue(catcher(void 0, 123) instanceof TypeError);

