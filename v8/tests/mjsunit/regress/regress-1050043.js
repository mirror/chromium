function unsignedShiftRight(val, shift) {
  return val >>> shift;
}

assertEquals(        15, unsignedShiftRight(15, 0), "15 >>> 0");
assertEquals(         7, unsignedShiftRight(15, 1), "15 >>> 1");
assertEquals(         3, unsignedShiftRight(15, 2), "15 >>> 2");

assertEquals(4294967288, unsignedShiftRight(-8, 0), "-8 >>> 0");
assertEquals(2147483644, unsignedShiftRight(-8, 1), "-8 >>> 1");
assertEquals(1073741822, unsignedShiftRight(-8, 2), "-8 >>> 2");

assertEquals(         1, unsignedShiftRight(-8, 31), "-8 >>> 31");
assertEquals(4294967288, unsignedShiftRight(-8, 32), "-8 >>> 32");
assertEquals(2147483644, unsignedShiftRight(-8, 33), "-8 >>> 33");
assertEquals(1073741822, unsignedShiftRight(-8, 34), "-8 >>> 34");

assertEquals(2147483648, unsignedShiftRight(0x80000000, 0), "0x80000000 >>> 0");
assertEquals(1073741824, unsignedShiftRight(0x80000000, 1), "0x80000000 >>> 1");
assertEquals( 536870912, unsignedShiftRight(0x80000000, 2), "0x80000000 >>> 2");

assertEquals(1073741824, unsignedShiftRight(0x40000000, 0), "0x40000000 >>> 0");
assertEquals( 536870912, unsignedShiftRight(0x40000000, 1), "0x40000000 >>> 1");
assertEquals( 268435456, unsignedShiftRight(0x40000000, 2), "0x40000000 >>> 2");
