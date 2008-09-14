// A reduced test case from Acid3 test 95.
// When an object is assigned to an array length field,
// it is converted to a number.

function CheckSetArrayLength(x, expected) {
  var a = [];
  a.length = x;

  assertEquals("number", typeof a.length);
  assertEquals(expected, a.length);
}

CheckSetArrayLength(2147483648, 2147483648);
CheckSetArrayLength("2147483648", 2147483648);
CheckSetArrayLength(null, 0);
CheckSetArrayLength(false, 0);
CheckSetArrayLength(true, 1);
CheckSetArrayLength({valueOf : function() { return 42; }}, 42);
CheckSetArrayLength({toString : function() { return '42'; }}, 42);

// Test invalid values
assertThrows("var y = []; y.length = 'abc';");
assertThrows("var y = []; y.length = undefined;");
assertThrows("var y = []; y.length = {};");
assertThrows("var y = []; y.length = -1;");
assertThrows("var y = []; y.length = {valueOf:function() { throw new Error(); }};");
