// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple debug event handler which just counts the number of break points hit.
var break_point_hit_count;

function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break) {
    break_point_hit_count++;
  }
};

// Add the debug event listener.
Debug.addListener(listener);

// Test function.
function f() {a=1;b=2;};

// This tests enabeling and disabling of break points including the case
// with several break points in the same location.
break_point_hit_count = 0;

// Set a breakpoint in f.
bp1 = Debug.setBreakPoint(f);
f();
assertEquals(1, break_point_hit_count);

// Disable the breakpoint.
Debug.disableBreakPoint(bp1);
f();
assertEquals(1, break_point_hit_count);

// Enable the breakpoint.
Debug.enableBreakPoint(bp1);
f();
assertEquals(2, break_point_hit_count);

// Set another breakpoint in f at the same place.
bp2 = Debug.setBreakPoint(f);
f();
assertEquals(3, break_point_hit_count);

// Disable the second breakpoint.
Debug.disableBreakPoint(bp2);
f();
assertEquals(4, break_point_hit_count);

// Disable the first breakpoint.
Debug.disableBreakPoint(bp1);
f();
assertEquals(4, break_point_hit_count);

// Enable both breakpoints.
Debug.enableBreakPoint(bp1);
Debug.enableBreakPoint(bp2);
f();
assertEquals(5, break_point_hit_count);

// Disable the first breakpoint.
Debug.disableBreakPoint(bp1);
f();
assertEquals(6, break_point_hit_count);
