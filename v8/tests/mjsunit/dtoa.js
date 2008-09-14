// dtoa.c used to contain a bogus assertions that got triggered when
// passed very small numbers.  This test therefore used to fail in
// debug mode.

assertEquals(0, 1e-500);
