// Regression test for buganizer issue #1112051.
function f() { }
assertThrows("f.call.apply()");
assertThrows("f.call.apply(null)");
assertThrows("f.call.apply(null, [], 0)");
assertThrows("f.call.apply(null, [1,2,3,4,5,6,7,8,9], 0)");
