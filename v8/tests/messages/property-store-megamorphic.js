function A() { }
function B() { }
function set_foo(obj) { obj.foo = 4; }
set_foo(new A());
set_foo(new B());
set_foo(null);
