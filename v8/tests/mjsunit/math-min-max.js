// Test Math.min().

assertEquals(Number.POSITIVE_INFINITY, Math.min());
assertEquals(1, Math.min(1));
assertEquals(1, Math.min(1, 2));
assertEquals(1, Math.min(2, 1));
assertEquals(1, Math.min(1, 2, 3));
assertEquals(1, Math.min(3, 2, 1));
assertEquals(1, Math.min(2, 3, 1));

var o = {};
o.valueOf = function() { return 1; };
assertEquals(1, Math.min(2, 3, '1'));
assertEquals(1, Math.min(3, o, 2));
assertEquals(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY / Math.min(-0, +0));
assertEquals(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY / Math.min(+0, -0));
assertEquals(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY / Math.min(+0, -0, 1));
assertEquals(-1, Math.min(+0, -0, -1));
assertEquals(-1, Math.min(-1, +0, -0));
assertEquals(-1, Math.min(+0, -1, -0));
assertEquals(-1, Math.min(-0, -1, +0));



// Test Math.max().

assertEquals(Number.NEGATIVE_INFINITY, Math.max());
assertEquals(1, Math.max(1));
assertEquals(2, Math.max(1, 2));
assertEquals(2, Math.max(2, 1));
assertEquals(3, Math.max(1, 2, 3));
assertEquals(3, Math.max(3, 2, 1));
assertEquals(3, Math.max(2, 3, 1));

var o = {};
o.valueOf = function() { return 3; };
assertEquals(3, Math.max(2, '3', 1));
assertEquals(3, Math.max(1, o, 2));
assertEquals(Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY / Math.max(-0, +0));
assertEquals(Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY / Math.max(+0, -0));
assertEquals(Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY / Math.max(+0, -0, -1));
assertEquals(1, Math.max(+0, -0, +1));
assertEquals(1, Math.max(+1, +0, -0));
assertEquals(1, Math.max(+0, +1, -0));
assertEquals(1, Math.max(-0, +1, +0));