// When a property of the arguments array is deleted, it
// must be "disconnected" from the corresponding parameter.
// Re-introducing the property does not connect to the parameter.

function f(x) {
  delete arguments[0];
  arguments[0] = 100;
  return x;
}

assertEquals(10, f(10));


/* This appears nasty at first glance. Probably not the most important
thing to fix at the moment. WHAT THE F**K WERE THEY THINKING??? (gri) */
