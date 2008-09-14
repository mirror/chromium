var foo = 'fisk';
assertEquals('fisk', foo);
var foo;
assertEquals('fisk', foo);
var foo = 'hest';
assertEquals('hest', foo);

this.bar = 'fisk';
assertEquals('fisk', bar);
var bar;
assertEquals('fisk', bar);
var bar = 'hest';
assertEquals('hest', bar);

this.baz = 'fisk';
assertEquals('fisk', baz);
eval('var baz;');
assertEquals('fisk', baz);
eval('var baz = "hest";');
assertEquals('hest', baz);
