function A(a) {
 if(a == true) {
  this.y = 234;
  this.z = 345;
 } else {
  this.z = 345;
  this.y = 234;
 }
}

var a = new A(true);
var b = new A(false);

print(a.y);
print(a.z);

print(b.y);
print(b.z);
