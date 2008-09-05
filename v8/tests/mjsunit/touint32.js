function ToUInt32(x) {
  return x >>> 0;
}

assertEquals(0, ToUInt32(0),         "0");
assertEquals(0, ToUInt32(-0),        "-0");
assertEquals(0, ToUInt32(Infinity),  "Infinity");
assertEquals(0, ToUInt32(-Infinity), "-Infinity");
assertEquals(0, ToUInt32(NaN),       "NaN");

assertEquals(0, ToUInt32(Number.MIN_VALUE),  "MIN");
assertEquals(0, ToUInt32(-Number.MIN_VALUE), "-MIN");
assertEquals(0, ToUInt32(0.1),               "0.1");
assertEquals(0, ToUInt32(-0.1),              "-0.1");
assertEquals(1, ToUInt32(1),                 "1");
assertEquals(1, ToUInt32(1.1),               "1.1");

assertEquals(4294967295, ToUInt32(-1),   "-1");
assertEquals(4294967295, ToUInt32(-1.1), "-1.1");

assertEquals(2147483647, ToUInt32(2147483647), "2147483647");
assertEquals(2147483648, ToUInt32(2147483648), "2147483648");
assertEquals(2147483649, ToUInt32(2147483649), "2147483649");

assertEquals(4294967295, ToUInt32(4294967295), "4294967295");
assertEquals(0,          ToUInt32(4294967296), "4294967296");
assertEquals(1,          ToUInt32(4294967297), "4294967297");

assertEquals(2147483649, ToUInt32(-2147483647), "-2147483647");
assertEquals(2147483648, ToUInt32(-2147483648), "-2147483648");
assertEquals(2147483647, ToUInt32(-2147483649), "-2147483649");

assertEquals(1,          ToUInt32(-4294967295), "-4294967295");
assertEquals(0,          ToUInt32(-4294967296), "-4294967296");
assertEquals(4294967295, ToUInt32(-4294967297), "-4294967297");

assertEquals(2147483647, ToUInt32('2147483647'), "'2147483647'");
assertEquals(2147483648, ToUInt32('2147483648'), "'2147483648'");
assertEquals(2147483649, ToUInt32('2147483649'), "'2147483649'");

assertEquals(4294967295, ToUInt32('4294967295'), "'4294967295'");
assertEquals(0,          ToUInt32('4294967296'), "'4294967296'");
assertEquals(1,          ToUInt32('4294967297'), "'4294967297'");


