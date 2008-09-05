// Check for objects.
assertTrue({x:12}.hasOwnProperty('x'));
assertFalse({x:12}.hasOwnProperty('y'));

// Check for strings.
assertTrue(''.hasOwnProperty('length'));
assertTrue(Object.prototype.hasOwnProperty.call('', 'length'));

// Check for numbers.
assertFalse((123).hasOwnProperty('length'));
assertFalse(Object.prototype.hasOwnProperty.call(123, 'length'));
