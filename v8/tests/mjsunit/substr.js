var s = 'abcdefghijklmn';
assertEquals(s, s.substr());
assertEquals(s, s.substr(0));
assertEquals(s, s.substr('0'));
assertEquals(s, s.substr(void 0));
assertEquals(s, s.substr(null));
assertEquals(s, s.substr(false));
assertEquals(s, s.substr(0.9));
assertEquals(s, s.substr({ valueOf: function() { return 0; } }));
assertEquals(s, s.substr({ toString: function() { return '0'; } }));

var s1 = s.substring(1);
assertEquals(s1, s.substr(1));
assertEquals(s1, s.substr('1'));
assertEquals(s1, s.substr(true));
assertEquals(s1, s.substr(1.1));
assertEquals(s1, s.substr({ valueOf: function() { return 1; } }));
assertEquals(s1, s.substr({ toString: function() { return '1'; } }));

for (var i = 0; i < s.length; i++)
  for (var j = i; j < s.length + 5; j++)
    assertEquals(s.substring(i, j), s.substr(i, j - i));

assertEquals(s.substring(s.length - 1), s.substr(-1));
assertEquals(s.substring(s.length - 1), s.substr(-1.2));
assertEquals(s.substring(s.length - 1), s.substr(-1.7));
assertEquals(s.substring(s.length - 2), s.substr(-2));
assertEquals(s.substring(s.length - 2), s.substr(-2.3));
assertEquals(s.substring(s.length - 2, s.length - 1), s.substr(-2, 1));
assertEquals(s, s.substr(-100));
assertEquals('abc', s.substr(-100, 3));
assertEquals(s1, s.substr(-s.length + 1));

// assertEquals('', s.substr(0, void 0)); // smjs and rhino 
assertEquals('abcdefghijklmn', s.substr(0, void 0));  // kjs and v8
assertEquals('', s.substr(0, null));
assertEquals(s, s.substr(0, String(s.length)));
assertEquals('a', s.substr(0, true));
