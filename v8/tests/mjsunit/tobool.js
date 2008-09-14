// All objects, including wrappers, must convert to true.
assertTrue(!!new Boolean(true), "new Boolean(true)");
assertTrue(!!new Boolean(false), "new Boolean(false)");

assertTrue(!!new Number(-1), "new Number(-1)");
assertTrue(!!new Number(0), "new Number(0)");
assertTrue(!!new Number(1), "new Number(1)");


