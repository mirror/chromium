// Ensure that literals are not overwritten.
function f1() { return (1.2, 3.4) + 5.6; }
function f2() { return (1, 2) + 3; }
function f3() { return (1.2 || 3.4) + 5.6; }
function f4() { return (1 || 2) + 3; }
assertTrue(f1() === f1());
assertTrue(f2() === f2());
assertTrue(f3() === f3());
assertTrue(f4() === f4());
