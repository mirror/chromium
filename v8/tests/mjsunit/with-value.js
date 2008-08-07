// Values used in 'with' statements should be wrapped in an object
// before establishing the context.
(function() {
  // 7 should be converted to an number object
  with (7) { assertTrue(typeof valueOf == 'function'); }
})();

/* This should be fairly easy again. May need some work in the
compiler's VisitWith() function, or perhaps the runtime routine's
PushContextForWith().
*/