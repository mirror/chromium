// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerComplete = false;
exception = false;
breakPointCount = 0;

function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      breakPointCount++;
      if (breakPointCount == 1) {
        // Break point in first with block.
        assertEquals(2, exec_state.GetFrame(0).evaluate('a').value());
        assertEquals(2, exec_state.GetFrame(0).evaluate('b').value());
      } else {
        // Break point in second with block.
        assertEquals(3, exec_state.GetFrame(0).evaluate('a').value());
        assertEquals(1, exec_state.GetFrame(0).evaluate('b').value());

        // Indicate that all was processed.
        listenerComplete = true;
      }
    }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

function f() {
  var a = 1;
  var b = 2;
  with ({a:2}) {
    debugger;  // Breakpoint.
    x = {a:3,b:1};
    with (x) {
      debugger;  // Breakpoint.
    }
  }
};

f();
// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete);
assertFalse(exception, "exception in listener")
