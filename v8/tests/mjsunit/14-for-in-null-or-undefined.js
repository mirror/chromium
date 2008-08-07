// At least Spidermonkey and IE allow for-in iteration over null and
// undefined. They never executed the statement block.
var count = 0;
for (var p in null) { count++; }
for (var p in void 0) { count++; }
assertEquals(0, count);
