function f() {
  return g();
};

function g() {
  var result = 0;
  var array = f.arguments;
  for (var i = 0; i < array.length; i++) {
    result += array[i];
  }
  return result;
};


// Make sure we can pass any number of arguments to f and read them
// from g.
for (var i = 0; i < 25; i++) {
  var array = new Array(i);
  var expected = 0;
  for (var j = 0; j < i; j++) {
    expected += j;
    array[j] = j;
  }
  assertEquals(expected, f.apply(null, array), String(i));
}


