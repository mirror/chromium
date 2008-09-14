function foo(x) {
  var a = arguments;
  function bar() {
    print(++a[0] + '(' + x + ')');
  };
  bar(); bar(); bar();
  return bar;
}
var baz = foo(0);
baz(); baz(); baz();
