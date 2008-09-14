function Foo(f) {
  this.f = f;
}

print(new Foo(function () { return 'foo'; }).f());
print(new Foo(function () { return 'bar'; }).f());

Function.prototype.x ="omsen"
Function.prototype.y ="momsen" 

print(Function.prototype());
print(Function.prototype.x)
print(Function.prototype.y)
print(Foo.x)
