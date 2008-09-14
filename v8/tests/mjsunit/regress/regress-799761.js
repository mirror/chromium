// const variables should be read-only
const c = 42;
c = 87;
assertEquals(42, c);


// const variables are not behaving like other JS variables when it comes
// to scoping - in fact they behave more sanely. Inside a 'with' they do
// not interfere with the 'with' scopes.

(function () {
  with ({ x: 42 }) {
    const x = 7;
  }
  x = 5;
  assertEquals(7, x);
})();


// const variables may be declared but never initialized, in which case
// their value is undefined.

(function (sel) {
  if (sel == 0)
    with ({ x: 42 }) {
    const x;
    }
  else
    x = 3;
  x = 5;
  assertTrue(typeof x == 'undefined');
})(1);


// const variables may be initialized to undefined.
(function () {
  with ({ x: 42 }) {
    const x = undefined;
  }
  x = 5;
  assertTrue(typeof x == 'undefined');
})();


// const variables may be accessed in inner scopes like any other variable.
(function () {
  function bar() {
    assertEquals(7, x);
  }
  with ({ x: 42 }) {
    const x = 7;
  }
  x = 5
  bar();
})();


// const variables may be declared via 'eval'
(function () {
  with ({ x: 42 }) {
    eval('const x = 7');
  }
  x = 5;
  assertEquals(7, x);
})();
