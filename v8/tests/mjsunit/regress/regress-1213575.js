// Copyright 2008 Google Inc. All Rights Reserved.

// Make sure that a const definition always
// conflicts with a defined setter. This avoid
// trying to pass 'the hole' to the setter.

this.__defineSetter__('x', function(value) { assertTrue(false); });

var caught = false;
try {
  eval('const x'); 
} catch(e) {
  assertTrue(e instanceof TypeError);
  caught = true;
}
assertTrue(caught);
