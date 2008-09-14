function toInt32(x) {
  return x | 0;
}

assertEquals(0, toInt32(Infinity));
assertEquals(0, toInt32(-Infinity));
assertEquals(0, toInt32(NaN));
assertEquals(0, toInt32(0.0));
assertEquals(0, toInt32(-0.0));

assertEquals(0, toInt32(Number.MIN_VALUE));
assertEquals(0, toInt32(-Number.MIN_VALUE));
assertEquals(0, toInt32(0.1));
assertEquals(0, toInt32(-0.1));
assertEquals(1, toInt32(1));
assertEquals(1, toInt32(1.1));
assertEquals(-1, toInt32(-1));

assertEquals(2147483647, toInt32(2147483647));
assertEquals(-2147483648, toInt32(2147483648));
assertEquals(-2147483647, toInt32(2147483649));

assertEquals(-1, toInt32(4294967295));
assertEquals(0, toInt32(4294967296));
assertEquals(1, toInt32(4294967297));

assertEquals(-2147483647, toInt32(-2147483647));
assertEquals(-2147483648, toInt32(-2147483648));
assertEquals(2147483647, toInt32(-2147483649));

assertEquals(1, toInt32(-4294967295));
assertEquals(0, toInt32(-4294967296));
assertEquals(-1, toInt32(-4294967297));

assertEquals(-2147483648, toInt32(2147483648.25));
assertEquals(-2147483648, toInt32(2147483648.5));
assertEquals(-2147483648, toInt32(2147483648.75));
assertEquals(-1, toInt32(4294967295.25));
assertEquals(-1, toInt32(4294967295.5));
assertEquals(-1, toInt32(4294967295.75));
assertEquals(-1294967296, toInt32(3000000000.25));
assertEquals(-1294967296, toInt32(3000000000.5));
assertEquals(-1294967296, toInt32(3000000000.75));

assertEquals(-2147483648, toInt32(-2147483648.25));
assertEquals(-2147483648, toInt32(-2147483648.5));
assertEquals(-2147483648, toInt32(-2147483648.75));
assertEquals(1, toInt32(-4294967295.25));
assertEquals(1, toInt32(-4294967295.5));
assertEquals(1, toInt32(-4294967295.75));
assertEquals(1294967296, toInt32(-3000000000.25));
assertEquals(1294967296, toInt32(-3000000000.5));
assertEquals(1294967296, toInt32(-3000000000.75));
