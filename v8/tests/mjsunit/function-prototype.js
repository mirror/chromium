// Test that we can set function prototypes to non-object values.  The
// prototype used for instances in that case should be the initial
// object prototype.  ECMA-262 13.2.2.
function TestNonObjectPrototype(value) {
  function F() {};
  F.prototype = value;
  var f = new F();
  assertEquals(value, F.prototype);
  assertEquals(Object.prototype, f.__proto__);
}

var values = [123, "asdf", true];

values.forEach(TestNonObjectPrototype);


// Test moving between non-object and object values.
function F() {};
var f = new F();
assertEquals(f.__proto__, F.prototype);
F.prototype = 42;
f = new F();
assertEquals(Object.prototype, f.__proto__);
assertEquals(42, F.prototype);
F.prototype = { a: 42 };
f = new F();
assertEquals(42, F.prototype.a);
assertEquals(f.__proto__, F.prototype);


// Test that the fast case optimizations can handle non-functions,
// functions with no prototypes (yet), non-object prototypes,
// functions without initial maps, and the fully initialized
// functions.
function GetPrototypeOf(f) {
  return f.prototype;
};

// Seed the GetPrototypeOf function to enable the fast case
// optimizations.
var p = GetPrototypeOf(GetPrototypeOf);

// Check that getting the prototype of a tagged integer works.
assertTrue(typeof GetPrototypeOf(1) == 'undefined');

function NoPrototypeYet() { }
var p = GetPrototypeOf(NoPrototypeYet);
assertEquals(NoPrototypeYet.prototype, p);

function NonObjectPrototype() { }
NonObjectPrototype.prototype = 42;
assertEquals(42, GetPrototypeOf(NonObjectPrototype));

function NoInitialMap() { }
var p = NoInitialMap.prototype;
assertEquals(p, GetPrototypeOf(NoInitialMap));

// Check the standard fast case.
assertEquals(F.prototype, GetPrototypeOf(F));

// Check that getting the prototype of a non-function works. This must
// be the last thing we do because this will clobber the optimizations
// in GetPrototypeOf and go to a monomorphic IC load instead.
assertEquals(87, GetPrototypeOf({prototype:87}));

// Check the prototype is enumerable as specified in ECMA262, 15.3.5.2
var foo = new Function("return x");
var result  = ""
for (var n in foo) result += n;
assertEquals(result, "prototype");
