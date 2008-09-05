// Test array sort.

// Test counter-intuitive default number sorting.
function TestNumberSort() {
  var a = [ 200, 45, 7 ];
  // Default sort calls toString on each element and orders
  // lexicographically.
  a.sort();
  assertArrayEquals([ 200, 45, 7 ], a);
  // Sort numbers by value using a compare functions.
  a.sort(function(x, y) { return x - y; });
  assertArrayEquals([ 7, 45, 200 ], a);
}

TestNumberSort();


// Test lexicographical string sorting.
function TestStringSort() {
  var a = [ "cc", "c", "aa", "a", "bb", "b", "ab", "ac" ];
  a.sort();
  assertArrayEquals([ "a", "aa", "ab", "ac", "b", "bb", "c", "cc" ], a);
}

TestStringSort();


// Test object sorting.  Calls toString on each element and sorts
// lexicographically.
function TestObjectSort() {
  var obj0 = { toString: function() { return "a"; } };
  var obj1 = { toString: function() { return "b"; } };
  var obj2 = { toString: function() { return "c"; } };
  var a = [ obj2, obj0, obj1 ];
  a.sort();
  assertArrayEquals([ obj0, obj1, obj2 ], a);
}

TestObjectSort();
