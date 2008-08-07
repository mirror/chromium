// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple object.
var a = {};

// Create mirror for the object.
var mirror = debug.MakeMirror(a);

// Initially one reference from the global object.
assertEquals(1, mirror.referencedBy().length);
assertEquals(1, mirror.referencedBy(0).length);
assertEquals(1, mirror.referencedBy(1).length);
assertEquals(1, mirror.referencedBy(10).length);

// Add some more references from simple objects and arrays.
var b = {}
b.a = a;
assertEquals(2, mirror.referencedBy().length);
var c = {}
c.a = a;
c.aa = a;
c.aaa = a;
assertEquals(3, mirror.referencedBy().length);
function d(){};
d.a = a
assertEquals(4, mirror.referencedBy().length);
e = [a,b,c,d];
assertEquals(5, mirror.referencedBy().length);


// Simple closure.
function closure_simple(p) {
  return function() { p = null; };
}

// This adds a reference (function context).
f = closure_simple(a);
assertEquals(6, mirror.referencedBy().length);
// This clears the reference (in function context).
f()
assertEquals(5, mirror.referencedBy().length);

// Use closure with eval - creates arguments array.
function closure_eval(p, s) {
  if (s) {
    eval(s);
  }
  return function e(s) { eval(s); };
}

// This adds a references (function context).
g = closure_eval(a);
assertEquals(6, mirror.referencedBy().length);

// Dynamically create a variable. This should create a context extension.
h = closure_eval(null, "var x_");
assertEquals(6, mirror.referencedBy().length);
// Adds a reference when set.
h("x_ = a");
var x = mirror.referencedBy();
// TODO(sgjesse) This should be 7 and not 8. 8 is caused by the context
// extension object beeing part of the result.
assertEquals(8, mirror.referencedBy().length);
// Removes a reference when cleared.
h("x_ = null");
assertEquals(6, mirror.referencedBy().length);

// Check circular references.
x = {}
mirror = debug.MakeMirror(x);
assertEquals(1, mirror.referencedBy().length);
x.x = x;
assertEquals(2, mirror.referencedBy().length);
x = null;
assertEquals(0, mirror.referencedBy().length);

// Check many references.
y = {}
mirror = debug.MakeMirror(y);
refs = [];
for (var i = 0; i < 200; i++) {
  refs[i] = {'y': y};
}
y = null;
assertEquals(200, mirror.referencedBy().length);
