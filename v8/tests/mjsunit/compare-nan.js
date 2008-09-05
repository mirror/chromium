var a = [NaN, -1, 0, 1, 1.2, -7.9, true, false, 'foo', '0', 'NaN' ];
for (var i in a) {
  var x = a[i];
  assertFalse(NaN == x); 
  assertFalse(NaN === x); 
  assertFalse(NaN < x);
  assertFalse(NaN > x);
  assertFalse(NaN <= x);
  assertFalse(NaN >= x);

  assertFalse(x == NaN); 
  assertFalse(x === NaN); 
  assertFalse(x < NaN);
  assertFalse(x > NaN);
  assertFalse(x <= NaN);
  assertFalse(x >= NaN);
}
