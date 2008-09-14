assertEquals(0, parseInt('0'));
assertEquals(0, parseInt(' 0'));
assertEquals(0, parseInt(' 0 '));

assertEquals(63, parseInt('077'));
assertEquals(63, parseInt('  077'));
assertEquals(63, parseInt('  077   '));
assertEquals(-63, parseInt('  -077'));

assertEquals(3, parseInt('11', 2));
assertEquals(4, parseInt('11', 3));

assertEquals(0x12, parseInt('0x12'));
assertEquals(0x12, parseInt('0x12', 16));

assertEquals(12, parseInt('12aaa'));

assertEquals(0.1, parseFloat('0.1'));
assertEquals(0.1, parseFloat('0.1aaa'));
assertEquals(0, parseFloat('0x12'));
assertEquals(77, parseFloat('077'));


var i;
var y = 10;

for (i = 1; i < 21; i++) {
  var x = eval("1.2e" + i);
  assertEquals(Math.floor(x), parseInt(x));
  x = eval("1e" + i);
  assertEquals(x, y);
  y *= 10;
  assertEquals(Math.floor(x), parseInt(x));
  x = eval("-1e" + i);
  assertEquals(Math.ceil(x), parseInt(x));
  x = eval("-1.2e" + i);
  assertEquals(Math.ceil(x), parseInt(x));
}

for (i = 21; i < 53; i++) {
  var x = eval("1e" + i);
  assertEquals(1, parseInt(x));
  x = eval("-1e" + i);
  assertEquals(-1, parseInt(x));
}

assertTrue(isNaN(parseInt(0/0)));
assertTrue(isNaN(parseInt(1/0)), "parseInt Infinity");
assertTrue(isNaN(parseInt(-1/0)), "parseInt -Infinity");

assertTrue(isNaN(parseFloat(0/0)));
assertEquals(Infinity, parseFloat(1/0), "parseFloat Infinity");
assertEquals(-Infinity, parseFloat(-1/0), "parseFloat -Infinity");


