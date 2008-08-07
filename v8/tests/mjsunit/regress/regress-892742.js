function f() {
  return/* useless*/1;
};


// According to ECMA-262, this comment should actually be parsed as a
// line terminator making g() return undefined, but this is not the
// way it's handled by Spidermonkey or KJS.
function g() {
  return/* useless
         */2;
};

function h() {
  return// meaningful
      3;
};


assertEquals(1, f());
assertEquals(2, g());
assertTrue(typeof h() == 'undefined', 'h');

