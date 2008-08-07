function assertClose(expected, actual) {
  var delta = 0.00001;
  if (Math.abs(expected - actual) > delta) {
    print('Failure: Expected <' + actual + '> to be close to <' + 
          expected + '>');    
  }
}

assertEquals(1, Math.pow(NaN, 0));
var pinf = Number.POSITIVE_INFINITY, ninf = Number.NEGATIVE_INFINITY;
assertClose( Math.PI / 4, Math.atan2(pinf, pinf));
assertClose(-Math.PI / 4, Math.atan2(ninf, pinf));
assertClose( 3 * Math.PI / 4, Math.atan2(pinf, ninf));
assertClose(-3 * Math.PI / 4, Math.atan2(ninf, ninf));

