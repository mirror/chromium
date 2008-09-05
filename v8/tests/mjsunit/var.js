assertTrue(!x && typeof x == 'undefined');
assertTrue(!y && typeof y == 'undefined');
if (false) { var x = 42; }
if (true)  { var y = 87; }
assertTrue(!x && typeof x == 'undefined');
assertEquals(87, y);

assertTrue(!z && typeof z == 'undefined');
if (false) { var z; }
assertTrue(!z && typeof z == 'undefined');
