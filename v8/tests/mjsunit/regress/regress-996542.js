var zero = 0;
var one = 1;
var minus_one = -1;

assertEquals(-Infinity, 1 / (0 / -1));
assertEquals(-Infinity, one / (zero / minus_one));
assertEquals(Infinity, 1 / (0 / 1));
assertEquals(Infinity, one / (zero / one));

assertEquals(-Infinity, 1 / (-1 % 1));
assertEquals(-Infinity, one / (minus_one % one))
assertEquals(Infinity, 1 / (1 % 1));
assertEquals(Infinity, one / (one % one));
