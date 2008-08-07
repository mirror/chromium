function Construct(x) { return x; }

assertFalse(null == new Construct(null));
assertFalse(void 0 == new Construct(void 0));
assertFalse(0 == new Construct(0));
assertFalse(1 == new Construct(1));
assertFalse(4.2 == new Construct(4.2));
assertFalse('foo' == new Construct('foo'));
assertFalse(true == new Construct(true));

x = {};
assertTrue(x === new Construct(x));
assertFalse(x === new Construct(null));
assertFalse(x === new Construct(void 0));
assertFalse(x === new Construct(1));
assertFalse(x === new Construct(3.2));
assertFalse(x === new Construct(false));
assertFalse(x === new Construct('bar'));
x = [];
assertTrue(x === new Construct(x));
x = new Boolean(true);
assertTrue(x === new Construct(x));
x = new Number(42);
assertTrue(x === new Construct(x));
x = new String('foo');
assertTrue(x === new Construct(x));
x = function() { };
assertTrue(x === new Construct(x));

