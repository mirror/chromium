print("for-in array");
var arr = new Array();
arr = [ 1, 2, 3, 4, 5, 6];
for (var i in arr) print(i);

print("for-in string");
var arr = "alpha";
for (var i in arr) print(i);

print("for-in string2");
var arr = new String("aa");
arr[6] = "sweet";
arr[7] = "oops";
for (var i in arr) print(i);
