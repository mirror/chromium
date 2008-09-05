var x = 0;
function Funky(a, b, c) { return 7; }
Number.prototype.__proto__ = Funky;
assertEquals(3, x.length);
assertEquals("Funky", x.name);
assertEquals(Funky.prototype, x.prototype);

Number.prototype.__proto__ = [1, 2, 3];
assertEquals(3, x.length);
