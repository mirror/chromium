function A() { this.foo = 4 }
function B() { this.foo = 18 }
function get_foo(obj) { return obj.foo; }
get_foo(new A());
get_foo(new B());
get_foo(null);
