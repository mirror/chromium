function props(x) {
  var array = [];
  for (var p in x) array.push(p);
  return array.sort();
}

assertEquals(0, props({}).length);
assertEquals(1, props({x:1}).length);
assertEquals(2, props({x:1, y:2}).length);

assertArrayEquals(["x"], props({x:1}));
assertArrayEquals(["x", "y"], props({x:1, y:2}));
assertArrayEquals(["x", "y", "zoom"], props({x:1, y:2, zoom:3}));

assertEquals(0, props([]).length);
assertEquals(1, props([1]).length);
assertEquals(2, props([1,2]).length);

assertArrayEquals(["0"], props([1]));
assertArrayEquals(["0", "1"], props([1,2]));
assertArrayEquals(["0", "1", "2"], props([1,2,3]));

var o = {};
var a = [];
for (var i = 0x0020; i < 0x01ff; i+=2) {
  var s = 'char:' + String.fromCharCode(i);
  a.push(s);
  o[s] = i;
}
assertArrayEquals(a, props(o));

var a = [];
assertEquals(0, props(a).length);
a[Math.pow(2,30)-1] = 0;
assertEquals(1, props(a).length);
a[Math.pow(2,31)-1] = 0;
assertEquals(2, props(a).length);
a[1] = 0;
assertEquals(3, props(a).length);

for (var hest = 'hest' in {}) { }
assertEquals('hest', hest);
