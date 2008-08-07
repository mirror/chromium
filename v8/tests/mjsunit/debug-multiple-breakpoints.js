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

// Test functions
function f() {a=1;b=2;};
function g() {f();}
function h() {}

// This test sets several break points at the same place and checks that
// several break points at the same place only makes one debug break event
// and that when the last break point is removed no more debug break events
// occours.
break_point_hit_count = 0;

// Set a breakpoint in f.
bp1 = Debug.setBreakPoint(f);
f();
assertEquals(1, break_point_hit_count);

// Set another breakpoint in f at the same place.
bp2 = Debug.setBreakPoint(f);
f();
assertEquals(2, break_point_hit_count);

// Remove one of the break points.
Debug.clearBreakPoint(bp1);
f();
assertEquals(3, break_point_hit_count);

// Remove the second break point.
Debug.clearBreakPoint(bp2);
f();
assertEquals(3, break_point_hit_count);

// Perform the same test using function g (this time removing the break points
// in the another order).
break_point_hit_count = 0;
bp1 = Debug.setBreakPoint(g);
g();
assertEquals(1, break_point_hit_count);
bp2 = Debug.setBreakPoint(g);
g();
assertEquals(2, break_point_hit_count);
Debug.clearBreakPoint(bp2);
g();
assertEquals(3, break_point_hit_count);
Debug.clearBreakPoint(bp1);
g();
assertEquals(3, break_point_hit_count);

// Finally test with many break points.
test_count = 100;
bps = new Array(test_count);
break_point_hit_count = 0;
for (var i = 0; i < test_count; i++) {
  bps[i] = Debug.setBreakPoint(h);
  h();
}
for (var i = 0; i < test_count; i++) {
  h();
  Debug.clearBreakPoint(bps[i]);
}
assertEquals(test_count * 2, break_point_hit_count);
h();
assertEquals(test_count * 2, break_point_hit_count);
