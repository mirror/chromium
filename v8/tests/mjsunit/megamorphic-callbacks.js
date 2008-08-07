function load(o) {
  return o.x;
};

function store(o) {
  o.y = 42;
};

function call(o) {
  return o.f();
};

// Create a slow-case object (with hashed properties).
var o = { x: 42, f: function() { }, z: 100 };
delete o.z;

// Initialize IC stubs.
load(o);
store(o);
call(o);


// Create a new slow-case object (with hashed properties) and add
// setter and getter properties to the object.
var o = { z: 100 };
delete o.z;
o.__defineGetter__("x", function() { return 100; });
o.__defineSetter__("y", function(value) { this.y_mirror = value; });
o.__defineGetter__("f", function() { return function() { return 300; }});

// Perform the load checks.
assertEquals(100, o.x, "normal load");
assertEquals(100, load(o), "ic load");

// Perform the store checks.
o.y = 200;
assertEquals(200, o.y_mirror, "normal store");
store(o);
assertEquals(42, o.y_mirror, "ic store");

// Perform the call checks.
assertEquals(300, o.f(), "normal call");
assertEquals(300, call(o), "ic call");
