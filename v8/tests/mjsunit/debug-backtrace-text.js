// Flags: --expose-debug-as debug

// The functions used for testing backtraces.
function Point(x, y) {
  this.x = x;
  this.y = y;
};

Point.prototype.distanceTo = function(p) {
  debugger;
  return Math.sqrt(Math.pow(Math.abs(this.x - p.x), 2) + Math.pow(Math.abs(this.y - p.y), 2))
}

p1 = new Point(1,1);
p2 = new Point(2,2);

p1.distanceTo = function(p) {
  return p.distanceTo(this);
}

function distance(p, q) {
  return p.distanceTo(q);
}

function createPoint(x, y) {
  return new Point(x, y);
}

a=[1,2,distance];

// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

testConstructor = false;  // Flag to control which part of the test is run.
listenerCalled = false;
exception = false;

function safeEval(code) {
  try {
    return eval('(' + code + ')');
  } catch (e) {
    return undefined;
  }
}

function listener(event, exec_state, event_data, data) {
  try {
  if (event == Debug.DebugEvent.Break)
  {
    if (!testConstructor) {
      // The expected backtrace is
      // 0: Call distance on Point where distance is a property on the prototype
      // 1: Call distance on Point where distance is a direct property
      // 2: Call on function an array element 2
      // 3: [anonymous]
      assertEquals("#<a Point>.distanceTo(p=#<a Point>)", exec_state.GetFrame(0).invocationText());
      assertEquals("#<a Point>.distanceTo(p=#<a Point>)", exec_state.GetFrame(1).invocationText());
      assertEquals("#<an Array>[2](aka distance)(p=#<a Point>, q=#<a Point>)", exec_state.GetFrame(2).invocationText());
      assertEquals("[anonymous]()", exec_state.GetFrame(3).invocationText());
      listenerCalled = true;
    } else {
      // The expected backtrace is
      // 0: Call Point constructor
      // 1: Call on global function createPoint
      // 2: [anonymous]
      assertEquals("new Point(x=0, y=0)", exec_state.GetFrame(0).invocationText());
      assertEquals("createPoint(x=0, y=0)", exec_state.GetFrame(1).invocationText());
      assertEquals("[anonymous]()", exec_state.GetFrame(2).invocationText());
      listenerCalled = true;
    }
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Set a break point and call to invoke the debug event listener.
a[2](p1, p2)

// Make sure that the debug event listener vas invoked.
assertTrue(listenerCalled);
assertFalse(exception, "exception in listener")

// Set a break point and call to invoke the debug event listener.
listenerCalled = false;
testConstructor = true;
Debug.setBreakPoint(Point, 0, 0);
createPoint(0, 0);

// Make sure that the debug event listener vas invoked (again).
assertTrue(listenerCalled);
assertFalse(exception, "exception in listener")
