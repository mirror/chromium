// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple debug event handler which counts the number of breaks hit and steps.
var break_break_point_hit_count = 0;
function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break) {
    break_break_point_hit_count++;
    // Continue stepping until returned to bottom frame.
    if (exec_state.GetFrameCount() > 1) {
      exec_state.prepareStep(Debug.StepAction.StepIn);
    }
    
    // Test that there is no script for the Date constructor.
    if (event_data.func().name() == 'Date') {
      assertTrue(typeof(event_data.func().script()) == 'undefined');
    } else {
      assertTrue(typeof(event_data.func().script()) == 'object');
    }
  }
};

// Add the debug event listener.
Debug.addListener(listener);

// Test step into constructor with simple constructor.
function X() {
}

function f() {
  debugger;
  new X();
};

break_break_point_hit_count = 0;
f();
assertEquals(5, break_break_point_hit_count);

// Test step into constructor with builtin constructor.
function g() {
  debugger;
  new Date();
};

break_break_point_hit_count = 0;
g();
assertEquals(5, break_break_point_hit_count);

// Get rid of the debug event listener.
Debug.removeListener(listener);