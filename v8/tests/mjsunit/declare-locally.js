// Make sure that we're not overwriting global
// properties defined in the prototype chain too
// early when shadowing them with var/const
// declarations.

// This exercises the code in runtime.cc in
// DeclareGlobal...Locally().

this.__proto__.foo = 42;
this.__proto__.bar = 87;

eval("assertEquals(42, foo); var foo = 87;");
assertEquals(87, foo);

eval("assertEquals(87, bar); const bar = 42;");
assertEquals(42, bar);
