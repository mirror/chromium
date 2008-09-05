function f() {
  function g() {}
}

print(this.g);  // should print 'undefined' (used to print g because it was defined globally)
