// Options:  --standardize-messages

function Test() {

}

Test.prototype.m2 = function() {
  String.foo.bar.baz;
}

Test.prototype.m1 = function() {
  return this.m2();
}

function foo() {
  (new Test).m1();
}

function bar() {
  foo();
}

bar();
