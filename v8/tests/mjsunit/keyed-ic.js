// This test attempts to test the inline caching for keyed access.

// ----------------------------------------------------------------------
// Prototype accessor.
// ----------------------------------------------------------------------
var runTest = function() {
  var initial_P = 'prototype';
  var P = initial_P;
  var H = 'hasOwnProperty';

  var f = function() {};

  function prototypeTest(change_index) {
    for (var i = 0; i < 10; i++) {
      var property = f[P];
      if (i <= change_index) {
        assertEquals(f.prototype, property);
      } else {
        assertEquals(f.hasOwnProperty, property);
      }
      if (i == change_index) P = H;
    }
    P = initial_P;
  }

  for (var i = 0; i < 10; i++) prototypeTest(i);

  f.prototype = 43;

  for (var i = 0; i < 10; i++) prototypeTest(i);
}

runTest();

// ----------------------------------------------------------------------
// Array length accessor.
// ----------------------------------------------------------------------
runTest = function() {
  var initial_L = 'length';
  var L = initial_L;
  var zero = '0';

  var a = new Array(10);

  function arrayLengthTest(change_index) {
    for (var i = 0; i < 10; i++) {
      var l = a[L];
      if (i <= change_index) {
        assertEquals(10, l);
      } else {
        assertEquals(undefined, l);
      }
      if (i == change_index) L = zero;
    }
    L = initial_L;
  }

  for (var i = 0; i < 10; i++) arrayLengthTest(i);
}

runTest();

// ----------------------------------------------------------------------
// String length accessor.
// ----------------------------------------------------------------------
runTest = function() {
  var initial_L = 'length';
  var L = initial_L;
  var zero = '0';

  var s = "asdf"

  function stringLengthTest(change_index) {
    for (var i = 0; i < 10; i++) {
      var l = s[L];
      if (i <= change_index) {
        assertEquals(4, l);
      } else {
        assertEquals('a', l);
      }
      if (i == change_index) L = zero;
    }
    L = initial_L;
  }

  for (var i = 0; i < 10; i++) stringLengthTest(i);
}

runTest();

// ----------------------------------------------------------------------
// Field access.
// ----------------------------------------------------------------------
runTest = function() {
  var o = { x: 42, y: 43 }

  var initial_X = 'x';
  var X = initial_X;
  var Y = 'y';

  function fieldTest(change_index) {
    for (var i = 0; i < 10; i++) {
      var property = o[X];
      if (i <= change_index) {
        assertEquals(42, property);
      } else {
        assertEquals(43, property);
      }
      if (i == change_index) X = Y;
    }
    X = initial_X;
  };

  for (var i = 0; i < 10; i++) fieldTest(i);
}

runTest();


// ----------------------------------------------------------------------
// Constant function access.
// ----------------------------------------------------------------------
runTest = function() {
  function fun() { };

  var o = new Object();
  o.f = fun;
  o.x = 42;

  var initial_F = 'f';
  var F = initial_F;
  var X = 'x'

  function constantFunctionTest(change_index) {
    for (var i = 0; i < 10; i++) {
      var property = o[F];
      if (i <= change_index) {
        assertEquals(fun, property);
      } else {
        assertEquals(42, property);
      }
      if (i == change_index) F = X;
    }
    F = initial_F;
  };

  for (var i = 0; i < 10; i++) constantFunctionTest(i);
}

runTest();

// ----------------------------------------------------------------------
// Keyed store field.
// ----------------------------------------------------------------------

runTest = function() {
  var o = { x: 42, y: 43 }

  var initial_X = 'x';
  var X = initial_X;
  var Y = 'y';

  function fieldTest(change_index) {
    for (var i = 0; i < 10; i++) {
      o[X] = X;
      var property = o[X];
      if (i <= change_index) {
        assertEquals('x', property);
      } else {
        assertEquals('y', property);
      }
      if (i == change_index) X = Y;
    }
    X = initial_X;
  };

  for (var i = 0; i < 10; i++) fieldTest(i);
}

runTest();
