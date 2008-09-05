function catchThese() {
  L: {
    try {
      break L;
    } catch (e) {}
  }
}

function finallyThese() {
  L: {
    try {
      break L;
    } finally {}
  }
}


for (var i = 0; i < 10; i++) {
  catchThese();
  gc();
}

for (var j = 0; j < 10; j++) {
  finallyThese();
  gc();
}

assertEquals(10, i);
assertEquals(10, j);


