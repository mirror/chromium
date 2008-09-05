// It should be possible to delete properties of 'with' context
// objects from within 'with' statements.
(function(){
  var tmp = { x: 12 };
  with (tmp) { assertTrue(delete x); }  
  assertFalse("x" in tmp);
})();

/*
This shouldn't be hard but may require some way to get hold
of the context representation in JS.
*/
