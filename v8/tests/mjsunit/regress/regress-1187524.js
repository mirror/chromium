// Make sure we don't die on conversion to Smi in string indexing

assertEquals(undefined, ""[0x40000000]);
assertEquals(undefined, ""[0x80000000]);
assertEquals(undefined, ""[-1]);
assertEquals(undefined, ""[-0x40000001]);
assertEquals(undefined, ""[-0x80000000]);
