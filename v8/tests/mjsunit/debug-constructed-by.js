// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple constructor.
function Point(x,y) {}

// Create mirror for the constructor.
var ctor = debug.MakeMirror(Point);

// Initially no instances.
assertEquals(0, ctor.constructedBy().length);
assertEquals(0, ctor.constructedBy(0).length);
assertEquals(0, ctor.constructedBy(1).length);
assertEquals(0, ctor.constructedBy(10).length);

// Create an instance.
var p = new Point();
assertEquals(1, ctor.constructedBy().length);
assertEquals(1, ctor.constructedBy(0).length);
assertEquals(1, ctor.constructedBy(1).length);
assertEquals(1, ctor.constructedBy(10).length);


// Create 10 more instances making for 11.
ps = [];
for (var i = 0; i < 10; i++) {
  ps.push(new Point());
}
assertEquals(11, ctor.constructedBy().length);
assertEquals(11, ctor.constructedBy(0).length);
assertEquals(1, ctor.constructedBy(1).length);
assertEquals(10, ctor.constructedBy(10).length);
