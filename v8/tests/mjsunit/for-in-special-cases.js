function for_in_null() {
  try {
    for (var x in null) {
      return false;
    }
  } catch(e) {
    return false;
  }
  return true;
}

function for_in_undefined() {
  try {
    for (var x in undefined) {
      return false;
    }
  } catch(e) {
    return false;
  }
  return true;
}

for (var i = 0; i < 10; ++i) {
  assertTrue(for_in_null());
  gc();
}

for (var j = 0; j < 10; ++j) {
  assertTrue(for_in_undefined());
  gc();
}

assertEquals(10, i);
assertEquals(10, j);

