// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple function which collects a simple call graph.
var call_graph = "";
function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break)
  {
    call_graph += exec_state.GetFrame().func().name();
    exec_state.prepareStep();
  }
};

// Add the debug event listener.
Debug.addListener(listener);

// Test debug event for constructor.
function a() {
  new c();
}

function b() {
  x = 1;
  new c();
}

function c() {
  this.x = 1;
  d();
}

function d() {
}

// Break point stops on "new c()" and steps into c.
Debug.setBreakPoint(a, 1);
call_graph = "";
a();
Debug.clearStepping();  // Clear stepping as the listener leaves it on.
assertEquals("accdca", call_graph);

// Break point stops on "x = 1" and steps to "new c()" and then into c.
Debug.setBreakPoint(b, 1);
call_graph = "";
b();
Debug.clearStepping();  // Clear stepping as the listener leaves it on.
assertEquals("bbccdcb", call_graph);

// Get rid of the debug event listener.
Debug.removeListener(listener);