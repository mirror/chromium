// Number
assertTrue(typeof 0 == 'number');
assertTrue(typeof 0 === 'number');
assertTrue(typeof 1.2 == 'number');
assertTrue(typeof 1.2 === 'number');
assertFalse(typeof 'x' == 'number');
assertFalse(typeof 'x' === 'number');

// String
assertTrue(typeof 'x' == 'string');
assertTrue(typeof 'x' === 'string');
assertTrue(typeof ('x' + 'x') == 'string');
assertTrue(typeof ('x' + 'x') === 'string');
assertFalse(typeof 1 == 'string');
assertFalse(typeof 1 === 'string');
assertFalse(typeof Object() == 'string');
assertFalse(typeof Object() === 'string');

// Boolean
assertTrue(typeof true == 'boolean');
assertTrue(typeof true === 'boolean');
assertTrue(typeof false == 'boolean');
assertTrue(typeof false === 'boolean');
assertFalse(typeof 1 == 'boolean');
assertFalse(typeof 1 === 'boolean');
assertFalse(typeof Object() == 'boolean');
assertFalse(typeof Object() === 'boolean');

// Undefined
assertTrue(typeof void 0 == 'undefined');
assertTrue(typeof void 0 === 'undefined');
assertFalse(typeof 1 == 'undefined');
assertFalse(typeof 1 === 'undefined');
assertFalse(typeof Object() == 'undefined');
assertFalse(typeof Object() === 'undefined');

// Function
assertTrue(typeof Object == 'function');
assertTrue(typeof Object === 'function');
assertFalse(typeof 1 == 'function');
assertFalse(typeof 1 === 'function');
assertFalse(typeof Object() == 'function');
assertFalse(typeof Object() === 'function');

// Object
assertTrue(typeof Object() == 'object');
assertTrue(typeof Object() === 'object');
assertTrue(typeof new String('x') == 'object');
assertTrue(typeof new String('x') === 'object');
assertTrue(typeof ['x'] == 'object');
assertTrue(typeof ['x'] === 'object');
assertTrue(typeof null == 'object');
assertTrue(typeof null === 'object');
assertFalse(typeof 1 == 'object');
assertFalse(typeof 1 === 'object');
assertFalse(typeof 'x' == 'object');  // bug #674753
assertFalse(typeof 'x' === 'object');
assertFalse(typeof Object == 'object');
assertFalse(typeof Object === 'object');

