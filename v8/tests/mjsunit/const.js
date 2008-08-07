// Test const properties and pre/postfix operation.
function f() {
  const x = 1;
  x++;
  assertEquals(1, x);
  x--;
  assertEquals(1, x);
  ++x;
  assertEquals(1, x);
  --x;
  assertEquals(1, x);
  assertEquals(1, x++);
  assertEquals(1, x--);
  assertEquals(2, ++x);
  assertEquals(0, --x);
}

f();

// Test that the value is read eventhough assignment is disallowed.
// Spidermonkey does not do this, but it seems like the right thing to
// do so that 'o++' is equivalent to 'o = o + 1'.
var valueOfCount = 0;

function g() {
  const o = { valueOf: function() { valueOfCount++; return 42; } }
  assertEquals(42, o);
  assertEquals(0, valueOfCount);
  o++;
  assertEquals(42, o);
  assertEquals(1, valueOfCount);
  ++o;
  assertEquals(42, o);
  assertEquals(2, valueOfCount);
  o--;
  assertEquals(42, o);
  assertEquals(3, valueOfCount);
  --o;
  assertEquals(42, o);
  assertEquals(4, valueOfCount);
}
