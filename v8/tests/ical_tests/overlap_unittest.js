function testRectComparatorY() {
  var x1 = { x:1, y:1, w:1, h:1};
  var x2 = { x:2, y:1, w:1, h:1};
  var x3 = { x:2, y:2, w:2, h:1};
  var x4 = { x:2, y:2, w:1, h:2};
  
  var x = [];
  x.push(x2, x3, x1, x4);
  x.sort(rectComparatorY_);
  
  assertEquals(x[0], x1);
  assertEquals(x[1], x2);
  assertEquals(x[2], x3);
  assertEquals(x[3], x4);
}

function testRectComparatorX() {
  var x1 = { x:1, y:1, w:1, h:1};
  var x2 = { x:1, y:2, w:1, h:1};
  var x3 = { x:2, y:2, w:1, h:2};
  var x4 = { x:2, y:2, w:2, h:1};
  
  var x = [];
  x.push(x2, x3, x1, x4);
  x.sort(rectComparatorX_);
  
  assertEquals(x[0], x1);
  assertEquals(x[1], x2);
  assertEquals(x[2], x3);
  assertEquals(x[3], x4);
}