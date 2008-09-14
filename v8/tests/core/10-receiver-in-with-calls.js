// When invoking functions from within a 'with' statement, we must set
// the receiver to the object where we found the function.

(function () {
  var x = { get_this: function() { return this; } };
  print(x === x.get_this());
  with (x) print(x === get_this());
})();


print({ f: function() {
  function g() { return this; };
  return eval("g")();
} }.f() == this);


print({ f: function() {
  function g() { return this; };
  return eval("g()");
} }.f() == this);
