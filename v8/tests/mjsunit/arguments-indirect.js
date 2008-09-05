function f1() {
  g(f1);
}

function f2(x) {
  var a = arguments;
  x++;
  g(f2);
}


function g(f) {
  assertEquals(3, f.arguments.length);
  assertEquals(1, f.arguments[0]);
  assertEquals(2, f.arguments[1]);
  assertEquals(3, f.arguments[2]);
}

f1(1,2,3);
f2(0,2,3);



/********************************************************
 * DISABLED PART OF TEST. WE CANNOT CURRENTLY HANDLE
 * THIS BECAUSE WE COPY THE ARGUMENTS OBJECT ON ACCESS.

function g(f) {
  assertEquals(100, f.arguments = 100);  // read-only
  assertEquals(3, f.arguments.length);
  assertEquals(1, f.arguments[0]);
  assertEquals(2, f.arguments[1]);
  assertEquals(3, f.arguments[2]);
  f.arguments[0] = 999;
  f.arguments.extra = 'kallevip';
}

function h(f) {
  assertEquals('kallevip', f.arguments.extra);
  return f.arguments;
}

// Test function with a materialized arguments array.
function f0() {
  g(f0);
  var result = h(f0);
  var a = arguments;
  assertEquals(999, a[0]);
  return result;
}


// Test function without a materialized arguments array.
function f1(x) {
  g(f1);
  var result = h(f1);
  assertEquals(999, x);
  return result;
}


function test(f) {
  assertTrue(null === f.arguments);
  var args = f(1,2,3);
  assertTrue(null === f.arguments);

  assertEquals(3, args.length);
  assertEquals(999, args[0]);
  assertEquals(2, args[1]);
  assertEquals(3, args[2]);
  assertEquals('kallevip', args.extra);
}

test(f0);
test(f1);




function w() {
  return q.arguments;
}

function q(x, y) {
  x = 2;
  var result = w();
  y = 3;
  return result;
}

var a = q(0, 1);
// x is set locally *before* the last use of arguments before the
// activation of q is popped from the stack.
assertEquals(2, a[0]);
// y is set locally *after* the last use of arguments before the
// activation of q is popped from the stack.
assertEquals(1, a[1]);


*
*************************************************************************/
