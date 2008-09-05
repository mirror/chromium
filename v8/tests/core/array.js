var a = new Array();
for (var n = 0; n < 100; ++n) {
  a[n] = n;
}
print(a[0]);
print(a[55]);
print(a[99]);

var z = [ 12 ];
print(z[0]);

z = [ 2.2 ];
print(z[0]);

z = [ z[0] ];
print(z[0]);

z = [ 3, 4, 5 ];
print(z[0]);
print(z[1]);
print(z[2]);

z = [ (function () { return 42; })(), 'foo', 7 + 9 ];
print(z[0]);
print(z[1]);
print(z[2]);

z = [ 0,, 2,,,, 6 ];
for (var i = 0; i < 7; i++) print(z[i]);

// Check the accessor length
var fisk = new Array(5);
print(fisk.length);


print([].sort());
print([1].sort());
print([1,2].sort());
print([2,1].sort());
print([1,2,3].sort());
print([3,2,1].sort());
print([2,3,1].sort());
print([3,1,2].sort());
print([0,0].sort());
print([0,-1,1].sort());

var a = [];
for (var i = 0; i < 100; i++) a.push(Math.random());
a.sort();
var p = a[0];
for (var i = 1; i < a.length; i++) {
  if (p > a[i]) print('array not sorted');
  p = a[i];
}
var b = [];
for (var i = 0; i < a.length; i++) b.push(a[i]);
b.sort();
print(a.toString() == b.toString());


// Tests array keys mixing strings and numbers.
var f = new Array();
f["123456"] = "yes";
print(f[123456]);

f = new Array();
f[123456] = "yes";
print(f["123456"]);

f = new Array(1,2);
f["1"] = "yes";
print(f[1]);

f = new Array(1,2);
f[1] = "yes";
print(f["1"]);
