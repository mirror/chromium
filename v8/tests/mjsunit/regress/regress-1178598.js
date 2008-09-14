// Regression test cases for issue 1178598.

// Make sure const-initialization doesn't conflict
// with heap-allocated locals for catch variables.
var value = (function(){
  try { } catch(e) {
    // Force the 'e' variable to be heap-allocated
    // by capturing it in a function closure.
    (function() { e; });
  }
  // Make sure the two definitions of 'e' do
  // not conflict in any way.
  eval("const e=1");
  return e;
})();

assertEquals(1, value);



// Make sure that catch variables can be accessed using eval.
var value = (function() {
  var result;
  try {
    throw 42;
  } catch (e) {
    result = eval("e");
  }
  return result;
})();

assertEquals(42, value);



// Make sure that heap-allocated locals for catch variables aren't
// visible outside the catch scope and that they are visible from
// within.
var value = (function() {
  var result;
  try {
    throw 87;
  } catch(e) {
    // Force the 'e' variable to be heap-allocated
    // by capturing it in a function closure.
    (function() { e; });
    result = eval("e");
  }

  // Expect accessing 'e' to yield an exception because
  // it is not defined in the current scope.
  try {
    eval("e");
    assertTrue(false);  // should throw exception
  } catch(exception) {
    assertTrue(exception instanceof ReferenceError);
    return result;
  }
})();

assertEquals(87, value);


