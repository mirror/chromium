// Flags: --allow-natives-syntax

// Make sure that we can introduce global variables (using
// both var and const) that shadow even READ_ONLY variables
// in the prototype chain.
const NONE = 0;
const READ_ONLY = 1;

// Use DeclareGlobal...
%AddProperty(this.__proto__, "a", "1234", NONE);
assertEquals(1234, a);
eval("var a = 5678;");
assertEquals(5678, a);

%AddProperty(this.__proto__, "b", "1234", NONE);
assertEquals(1234, b);
eval("const b = 5678;");
assertEquals(5678, b);

%AddProperty(this.__proto__, "c", "1234", READ_ONLY);
assertEquals(1234, c);
eval("var c = 5678;");
assertEquals(5678, c);

%AddProperty(this.__proto__, "d", "1234", READ_ONLY);
assertEquals(1234, d);
eval("const d = 5678;");
assertEquals(5678, d);

// Use DeclareContextSlot...
%AddProperty(this.__proto__, "x", "1234", NONE);
assertEquals(1234, x);
eval("with({}) { var x = 5678; }");
assertEquals(5678, x);

%AddProperty(this.__proto__, "y", "1234", NONE);
assertEquals(1234, y);
eval("with({}) { const y = 5678; }");
assertEquals(5678, y);

%AddProperty(this.__proto__, "z", "1234", READ_ONLY);
assertEquals(1234, z);
eval("with({}) { var z = 5678; }");
assertEquals(5678, z);

%AddProperty(this.__proto__, "w", "1234", READ_ONLY);
assertEquals(1234, w);
eval("with({}) { const w = 5678; }");
assertEquals(5678, w);


