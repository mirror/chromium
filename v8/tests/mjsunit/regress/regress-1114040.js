function TestBreak() {
  var sequence = "";
  for (var a in [0,1]) {
    L: {
      for (var b in [2,3,4]) {
        break L;
      }
    }
    sequence += a;
  }
  return sequence;
}


function TestContinue() {
  var sequence = "";
  for (var a in [0,1]) {
    L: do {
      for (var b in [2,3,4]) {
        continue L;
      }
    } while (false);
    sequence += a;
  }
  return sequence;
}


assertEquals("01", TestBreak());
assertEquals("01", TestContinue());

