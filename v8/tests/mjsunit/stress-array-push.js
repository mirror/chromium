// Rewrite of mozilla_js_tests/js1_5/GC/regress-278725
// to stress test pushing elements to an array.
var results = [];
for (var k = 0; k < 60000; k++) {
  if ((k%10000) == 0) results.length = 0;
  results.push({});
}
