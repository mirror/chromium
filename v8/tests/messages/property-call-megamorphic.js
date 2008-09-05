function A() { this.foo = function () { } }
function B() { this.foo = function () { } }
function call_foo(obj) { return obj.foo(); }
call_foo(new A());
call_foo(new B());
call_foo(null);
