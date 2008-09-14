function A() { this.foo = 4 }
function get_foo(obj) { return obj.foo; }
get_foo(new A());
get_foo(null);
