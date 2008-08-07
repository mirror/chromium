/*
const semantics
---------------

1) The first encountered declaration of a name determines it's mode (var or const).
(Note that all "static" (i.e. source text) declarations are "executed" at function
entry, before any of the statements of the function are executed. That is, if there
is a static const declaration at the end of the function, it still precedes any
dynamic const declaration (via eval()) that may be encountered before.)

2) A const name cannot be redeclared. Redeclaration leads to a type error exception.
(In particular, a const name cannot be redeclared as const, even if the initial
expression is the same). The type error may be thrown at compile/parse time.

3) const names follow the same scoping rules as functions.
(In particular, they are subtly different from variables. For example:

  var obj = { a: 0, b: 1 };
  with (obj) {
    const a = 7;
    var b = 8;
  }
  // a == 7, obj.a == 0
  // b == undefined, obj.b == 8 (because "var v = x" => "var v; v = x")
)

4) The first executed initialization expression sets the value of a const; the
value is undefined before that. Example:

  function () {
    use(a);  // a is undefined
    const a = 7;
    use(a);  // a is 7
  }
  
  function () {
    use(a);  // a is undefined
    for (var i = -10; i < 10; i++) {
      const a = i;
      use(a);  // a is always -10
    }
    use(a);  // a is still -10
  }
  
Note that empty initialization (missing initialization expression) is legal
and initializes the name to undefined.

5) Assignments to const names are ignored.
*/


function check(x, y) {
  if (x != y) {
    throw("check failed: " + x + " != " + y);
  }
}


function Test(f) {
  try {
    f();
  } catch (x) {
    print("Test \"" + f + "\" failed with exception: " + x);
  }
}


// Sanity check
Test(
  function () {
    check(0, 1);
  }
);


// ----------------------------------------------------------------------------
// Tests

// assignment to const is ignored
Test(
  function () {
    check(a, undefined);
    const a = 0;
    check(a, 0);
    a = 1;
    check(a, 0);
  }
);


Test(
  function () {
    check(a, undefined);
    const a = 0;
    check(a, 0);
    eval("a = 1;");
    check(a, 0);
  }
);



// assignment to const is ignored, but expression is still evaluated
Test(
  function () {
    var x = 0;
    function f() { x = 1; return x; }
    check(a, undefined);
    const a = 0;
    check(a, 0);
    a = f();
    check(a, 0);
    check(x, 1);
  }
);


Test(
  function () {
    var x = 0;
    function f() { x = 1; return x; }
    check(a, undefined);
    const a = 0;
    check(a, 0);
    eval("a = f();");
    check(a, 0);
    check(x, 1);
  }
);


// assignment to const is ignored
Test(
  function () {
    eval("const a = -1;");
    for (var i = 0; i < 10; i++) {
      check(a, -1);
      a = i;
    }
  }
);


Test(
  function () {
    const a = -1;
    for (var i = 0; i < 10; i++) {
      check(a, -1);
      a = i;
    }
  }
);


// first initialization matters
Test(
  function () {
    check(a, undefined);
    for (var i = 0; i < 10; i++) {
      const a = i;
      check(a, 0);
    }
  }
);


Test(
  function () {
    eval(
      "check(a, undefined);" +
      "for (var i = 0; i < 10; i++) {" +
      "  const a = i;" +
      "  check(a, 0);" +
      "}"
    );
  }
);


Test(
  function () {
    for (var i = 0; i < 10; i++) {
      check(a, i <= 3 ? undefined : 3);
      if (i > 2) {
        const a = i;
      }
    }
  }
)
