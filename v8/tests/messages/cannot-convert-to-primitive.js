function Foo() { }

Foo.prototype.toString = Foo.prototype.valueOf = function() { return this; }
print(new Foo());