with ({}) { }
this.bar = 'fisk';
assertEquals('fisk', bar);
var bar;
assertEquals('fisk', bar);
var bar = 'hest';
assertEquals('hest', bar);

with ({}) {
  this.baz = 'fisk';
  assertEquals('fisk', baz);
  var baz;
  assertEquals('fisk', baz);
  var baz = 'hest';
  assertEquals('hest', baz);
}
