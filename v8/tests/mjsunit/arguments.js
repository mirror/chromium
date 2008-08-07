function argc0() {
  return arguments.length;
}

function argc1(i) {
  return arguments.length;
}

function argc2(i, j) {
  return arguments.length;
}

assertEquals(0, argc0());
assertEquals(1, argc0(1));
assertEquals(2, argc0(1, 2));
assertEquals(3, argc0(1, 2, 3));
assertEquals(0, argc1());
assertEquals(1, argc1(1));
assertEquals(2, argc1(1, 2));
assertEquals(3, argc1(1, 2, 3));
assertEquals(0, argc2());
assertEquals(1, argc2(1));
assertEquals(2, argc2(1, 2));
assertEquals(3, argc2(1, 2, 3));



var index;

function argv0() {
  return arguments[index];
}

function argv1(i) {
  return arguments[index];
}

function argv2(i, j) {
  return arguments[index];
}

index = 0;
assertEquals(7, argv0(7));
assertEquals(7, argv0(7, 8));
assertEquals(7, argv0(7, 8, 9));
assertEquals(7, argv1(7));
assertEquals(7, argv1(7, 8));
assertEquals(7, argv1(7, 8, 9));
assertEquals(7, argv2(7));
assertEquals(7, argv2(7, 8));
assertEquals(7, argv2(7, 8, 9));

index = 1;
assertEquals(8, argv0(7, 8));
assertEquals(8, argv0(7, 8));
assertEquals(8, argv1(7, 8, 9));
assertEquals(8, argv1(7, 8, 9));
assertEquals(8, argv2(7, 8, 9));
assertEquals(8, argv2(7, 8, 9));

index = 2;
assertEquals(9, argv0(7, 8, 9));
assertEquals(9, argv1(7, 8, 9));
assertEquals(9, argv2(7, 8, 9));


// Test that calling a lazily compiled function with
// an unexpected number of arguments works.
function f(a) { return arguments.length; };
assertEquals(3, f(1, 2, 3));
