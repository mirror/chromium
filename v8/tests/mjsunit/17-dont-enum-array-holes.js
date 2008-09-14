// Do not enum holes in arrays.
var count = 0;
for (var i in [,1,,3]) count++;
assertEquals(2, count);

count = 0;
for (var i in new Array(10)) count++;
assertEquals(0, count);
