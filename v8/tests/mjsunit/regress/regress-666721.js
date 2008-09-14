function len0(a) { return a.length; }
function len1(a) { return a.length; }
function len2(a) { return a.length; }
function len3(a) { return a.length; }

assertEquals(0, len0([]));
assertEquals(1, len0({length:1}));
assertEquals(2, len0([1,2]));
assertEquals(3, len0('123'));

assertEquals(0, len1(''));
assertEquals(1, len1({length:1}));
assertEquals(2, len1('12'));
assertEquals(3, len1([1,2,3]));

assertEquals(0, len2({length:0}));
assertEquals(1, len2([1]));
assertEquals(2, len2({length:2}));
assertEquals(3, len2([1,2,3]));
assertEquals(4, len2('1234'));

assertEquals(0, len3({length:0}));
assertEquals(1, len3('1'));
assertEquals(2, len3({length:2}));
assertEquals(3, len3('123'));
assertEquals(4, len3([1,2,3,4]));
