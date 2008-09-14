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
function f() {};

// This tests ignore of break points including the case with several
// break points in the same location.
break_point_hit_count = 0;

// Set a breakpoint in f.
bp1 = Debug.setBreakPoint(f);

// Try ignore count of 1.
Debug.changeBreakPointIgnoreCount(bp1, 1);
f();
assertEquals(0, break_point_hit_count);
f();
assertEquals(1, break_point_hit_count);

// Set another breakpoint in f at the same place.
bp2 = Debug.setBreakPoint(f);
f();
assertEquals(2, break_point_hit_count);

// Set different ignore counts.
Debug.changeBreakPointIgnoreCount(bp1, 2);
Debug.changeBreakPointIgnoreCount(bp2, 4);
f();
assertEquals(2, break_point_hit_count);
f();
assertEquals(2, break_point_hit_count);
f();
assertEquals(3, break_point_hit_count);
f();
assertEquals(4, break_point_hit_count);

// Set different ignore counts (opposite).
Debug.changeBreakPointIgnoreCount(bp1, 4);
Debug.changeBreakPointIgnoreCount(bp2, 2);
f();
assertEquals(4, break_point_hit_count);
f();
assertEquals(4, break_point_hit_count);
f();
assertEquals(5, break_point_hit_count);
f();
assertEquals(6, break_point_hit_count);

