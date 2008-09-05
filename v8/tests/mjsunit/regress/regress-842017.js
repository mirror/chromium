function break_from_for_in() {
  L: {
    try {
      for (var x in [1,2,3]) {
        break L;
      }
    } finally {}
  }
}

function break_from_finally() {
  L: {
    try {
    } finally {
      break L;
    }
  }
}

for (var i = 0; i < 10; i++) {
  break_from_for_in();
  gc();
}

for (var j = 0; j < 10; j++) {
  break_from_finally();
  gc();
}

assertEquals(10, i);
assertEquals(10, j);
