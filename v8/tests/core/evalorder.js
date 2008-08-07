f();

function f() {
  print("f() called");
}


f = g;


function g() {
  print("g() called");
}

f();

var a = new Array(10);
var j = 0;
a[0]=0;
a[1]=1;
a[2]=2;
a[++j]++; // should incr. a[1]

print(a[0]);
print(a[1]);
print(a[2]);

var i = 0;
++a[i++]; // should incr. a[0]
++a[++i]; // should incr. a[2]

print(a[0]);
print(a[1]);
print(a[2]);

function h() {
  print('h() called');
  return { x: 0 };
}

(h()).x++;
++(h()).x;
