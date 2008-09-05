var o1 = { x: 12 };

var o2 = { x: 12, y: 13 };
delete o2.x;  // normalize

assertTrue(o1.__proto__ === o2.__proto__);
