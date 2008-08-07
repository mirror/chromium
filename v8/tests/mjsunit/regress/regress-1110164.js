var o = { x: 0, f: function() { return 42; } };
delete o.x;  // go dictionary

function CallF(o) {
  return o.f();
}

// Make sure the call IC in CallF is initialized.
for (var i = 0; i < 10; i++) assertEquals(42, CallF(o));

var caught = false;
o.f = 87;
try {
  CallF(o);
} catch (e) {
  caught = true;
  assertTrue(e instanceof TypeError);
}
assertTrue(caught);
