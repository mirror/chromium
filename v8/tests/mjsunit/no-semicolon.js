// Random tests to make sure you can leave out semicolons
// in various places.

function f() { return }

function g() { 
  return
    4;
}

assertTrue(f() === void 0);
assertTrue(g() === void 0);

for (var i = 0; i < 10; i++) { break }
assertEquals(0, i);

for (var i = 0; i < 10; i++) { continue }
assertEquals(10, i);