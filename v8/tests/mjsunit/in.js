// ----------------
// Check fast objects

var o = { };
assertFalse(0 in o);
assertFalse('x' in o);
assertFalse('y' in o);
assertTrue('toString' in o, "toString");

var o = { x: 12 };
assertFalse(0 in o);
assertTrue('x' in o);
assertFalse('y' in o);
assertTrue('toString' in o, "toString");

var o = { x: 12, y: 15 };
assertFalse(0 in o);
assertTrue('x' in o);
assertTrue('y' in o);
assertTrue('toString' in o, "toString");


// ----------------
// Check dense arrays

var a = [ ];
assertFalse(0 in a);
assertFalse(1 in a);
assertFalse('0' in a);
assertFalse('1' in a);
assertTrue('toString' in a, "toString");

var a = [ 1 ];
assertTrue(0 in a);
assertFalse(1 in a);
assertTrue('0' in a);
assertFalse('1' in a);
assertTrue('toString' in a, "toString");

var a = [ 1, 2 ];
assertTrue(0 in a);
assertTrue(1 in a);
assertTrue('0' in a);
assertTrue('1' in a);
assertTrue('toString' in a, "toString");

var a = [ 1, 2 ];
assertFalse(0.001 in a);
assertTrue(-0 in a);
assertTrue(+0 in a);
assertFalse('0.0' in a);
assertFalse('1.0' in a);
assertFalse(NaN in a);
assertFalse(Infinity in a);
assertFalse(-Infinity in a);

/*****
 * NOTE: Two of the tests below are disabled due to a bug in V8.
 * Fast case (non-dictionary) sparse arrays do not work as advertised.
 *
 */

var a = [];
a[1] = 2;
//assertFalse(0 in a);
assertTrue(1 in a);
assertFalse(2 in a);
//assertFalse('0' in a); 
assertTrue('1' in a);
assertFalse('2' in a);
assertTrue('toString' in a, "toString");


// ----------------
// Check dictionary ("normalized") objects

var o = {};
for (var i = 0x0020; i < 0x02ff; i += 2) {
  o['char:' + String.fromCharCode(i)] = i;
}
for (var i = 0x0020; i < 0x02ff; i += 2) {
  assertTrue('char:' + String.fromCharCode(i) in o);
  assertFalse('char:' + String.fromCharCode(i + 1) in o);
}
assertTrue('toString' in o, "toString");

var o = {};
o[Math.pow(2,30)-1] = 0;
o[Math.pow(2,31)-1] = 0;
o[1] = 0;
assertFalse(0 in o);
assertTrue(1 in o);
assertFalse(2 in o);
assertFalse(Math.pow(2,30)-2 in o);
assertTrue(Math.pow(2,30)-1 in o);
assertFalse(Math.pow(2,30)-0 in o);
assertTrue(Math.pow(2,31)-1 in o);
assertFalse(0.001 in o);
assertFalse('0.0' in o);
assertFalse('1.0' in o);
assertFalse(NaN in o);
assertFalse(Infinity in o);
assertFalse(-Infinity in o);
assertFalse(-0 in o);
assertFalse(+0 in o);
assertTrue('toString' in o, "toString");


// ----------------
// Check sparse arrays

var a = [];
a[Math.pow(2,30)-1] = 0;
a[Math.pow(2,31)-1] = 0;
a[1] = 0;
assertFalse(0 in a, "0 in a");
assertTrue(1 in a, "1 in a");
assertFalse(2 in a, "2 in a");
assertFalse(Math.pow(2,30)-2 in a, "Math.pow(2,30)-2 in a");
assertTrue(Math.pow(2,30)-1 in a, "Math.pow(2,30)-1 in a");
assertFalse(Math.pow(2,30)-0 in a, "Math.pow(2,30)-0 in a");
assertTrue(Math.pow(2,31)-1 in a, "Math.pow(2,31)-1 in a");
assertFalse(0.001 in a, "0.001 in a");
assertFalse('0.0' in a,"'0.0' in a");
assertFalse('1.0' in a,"'1.0' in a");
assertFalse(NaN in a,"NaN in a");
assertFalse(Infinity in a,"Infinity in a");
assertFalse(-Infinity in a,"-Infinity in a");
assertFalse(-0 in a,"-0 in a");
assertFalse(+0 in a,"+0 in a");
assertTrue('toString' in a, "toString");
