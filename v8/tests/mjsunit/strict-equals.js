var n = null;
var u = void 0;
assertTrue(null === null);
assertTrue(null === n);
assertTrue(n === null);
assertTrue(n === n);
assertFalse(null === void 0);
assertFalse(void 0 === null);
assertFalse(u === null);
assertFalse(null === u);
assertFalse(n === u);
assertFalse(u === n);
assertTrue(void 0 === void 0);
assertTrue(u === u);
assertTrue(u === void 0);
assertTrue(void 0 === u);

assertTrue('foo' === 'foo');
assertFalse('bar' === 'foo');
assertFalse('foo' === new String('foo'));
assertFalse(new String('foo') === new String('foo'));
var s = new String('foo');
assertTrue(s === s);
assertFalse(s === null);
assertFalse(s === void 0);
assertFalse('foo' === null);
assertFalse('foo' === 7);
assertFalse('foo' === true);
assertFalse('foo' === void 0);
assertFalse('foo' === {});

assertFalse({} === {});
var x = {};
assertTrue(x === x);
assertFalse(x === null);
assertFalse(x === 7);
assertFalse(x === true);
assertFalse(x === void 0);
assertFalse(x === {});

assertTrue(true === true);
assertTrue(false === false);
assertFalse(false === true);
assertFalse(true === false);
assertFalse(true === new Boolean(true));
assertFalse(true === new Boolean(false));
assertFalse(false === new Boolean(true));
assertFalse(false === new Boolean(false));
assertFalse(true === 0);
assertFalse(true === 1);

assertTrue(0 === 0);
assertTrue(-0 === -0);
assertTrue(-0 === 0);
assertTrue(0 === -0);
assertFalse(0 === new Number(0));
assertFalse(1 === new Number(1));
assertTrue(4.2 === 4.2);
assertTrue(4.2 === Number(4.2));




