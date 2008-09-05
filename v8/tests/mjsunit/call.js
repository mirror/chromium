function f0() {
  return this;
}

assertTrue(this === f0.call(), "1");

assertTrue(this === f0.call(this), "w");
assertTrue(this === f0.call(this, 1), "w");
assertTrue(this === f0.call(this, 1, 2), "w");

assertTrue(this === f0.call(null), "3a");
assertTrue(this === f0.call(null, 1), "3b");
assertTrue(this === f0.call(null, 1, 2), "3c");

assertTrue(this === f0.call(void 0), "4a");
assertTrue(this === f0.call(void 0, 1), "4b");
assertTrue(this === f0.call(void 0, 1, 2), "4c");

var x = {};
assertTrue(x === f0.call(x));
assertTrue(x === f0.call(x, 1));
assertTrue(x === f0.call(x, 1, 2));


function f1(a) {
  a = a || 'i';
  return this[a];
}

assertEquals(1, f1.call({i:1}));
assertEquals(42, f1.call({i:42}, 'i'));
assertEquals(87, f1.call({j:87}, 'j', 1));
assertEquals(99, f1.call({k:99}, 'k', 1, 2));


function f2(a, b) {
  a = a || 'n';
  b = b || 2;
  return this[a] + b;
}

var x = {n: 1};
assertEquals(3, f2.call(x));
assertEquals(14, f2.call({i:12}, 'i'));
assertEquals(42, f2.call(x, 'n', 41));
assertEquals(87, f2.call(x, 'n', 86, 1));
assertEquals(99, f2.call(x, 'n', 98, 1, 2));


function fn() {
  return arguments.length;
}

assertEquals(0, fn.call());
assertEquals(0, fn.call(this));
assertEquals(0, fn.call(null));
assertEquals(0, fn.call(void 0));
assertEquals(1, fn.call(this, 1));
assertEquals(2, fn.call(this, 1, 2));
assertEquals(3, fn.call(this, 1, 2, 3));
