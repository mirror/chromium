// Simple splice tests taken from webkit layout tests.
var arr = ['a','b','c','d'];
assertArrayEquals(['a','b','c','d'], arr);
assertArrayEquals(['c','d'], arr.splice(2));
assertArrayEquals(['a','b'], arr);
assertArrayEquals(['a','b'], arr.splice(0));
assertArrayEquals([], arr)

arr = ['a','b','c','d'];
assertEquals(undefined, arr.splice())
assertArrayEquals(['a','b','c','d'], arr);
assertArrayEquals(['a','b','c','d'], arr.splice(undefined))
assertArrayEquals([], arr);

arr = ['a','b','c','d'];
assertArrayEquals(['a','b','c','d'], arr.splice(null))
assertArrayEquals([], arr);

arr = ['a','b','c','d'];
assertArrayEquals([], arr.splice(100))
assertArrayEquals(['a','b','c','d'], arr);
assertArrayEquals(['d'], arr.splice(-1))
assertArrayEquals(['a','b','c'], arr);

assertArrayEquals([], arr.splice(2, undefined))
assertArrayEquals([], arr.splice(2, null))
assertArrayEquals([], arr.splice(2, -1))
assertArrayEquals([], arr.splice(2, 0))
assertArrayEquals(['a','b','c'], arr);
assertArrayEquals(['c'], arr.splice(2, 100))
assertArrayEquals(['a','b'], arr);


