function A() { }
function set_foo(obj) { obj.foo = 0; }
set_foo(new A());
set_foo(null);
