function XXX(x) {
  var k = delete x;
  return k;
}

assertFalse(XXX('Hello'));

