// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple debug event handler which first time hit will perform 1000 steps and
// second time hit will evaluate and store the value of "i". If requires that
// the global property "state" is initially zero.

var bp1, bp2;

function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break)
  {
    if (state == 0) {
      exec_state.prepareStep(Debug.StepAction.StepIn, 1000);
      state = 1;
    } else if (state == 1) {
      result = exec_state.GetFrame().evaluate("i").value();
      // Clear the break point on line 2 if set.
      if (bp2) {
        Debug.clearBreakPoint(bp2);
      }
    }
  }
};

// Add the debug event listener.
Debug.addListener(listener);

// Test debug event for break point.
function f() {
  for (i = 0; i < 1000; i++) {  //  Line 1.
    x = 1;                      //  Line 2.
  }
};

// Set a breakpoint on the for statement (line 1).
bp1 = Debug.setBreakPoint(f, 1);

// Check that performing 1000 steps will make i 499.
state = 0;
result = -1;
f();
print(state);
assertEquals(499, result);

// Check that performing 1000 steps with a break point on the statement in the
// for loop (line 2) will only make i 0 as a real break point breaks even when
// multiple steps have been requested.
state = 0;
result = -1;
bp2 = Debug.setBreakPoint(f, 2);
f();
assertEquals(0, result);

// Get rid of the debug event listener.
Debug.removeListener(listener);