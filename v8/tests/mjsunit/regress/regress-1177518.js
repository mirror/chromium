// Make sure that natives and delayed natives don't use methods from the global
// scope that could have been modified by input javascript.

isFinite = 0;
Math.floor = 0;
Math.abs = 0;

// uses Math.floor
assertEquals(4, parseInt(4.5));

// uses Math.abs, Math.floor and isFinite
assertEquals('string', typeof (new Date(9999)).toString());
