// Flags: --expose-debug-as debug
// Make sure that the retreival of local variables are performed correctly even
// when an adapter frame is present.

// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerCalled = false;
exception = false;

function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break) {
      assertEquals('c', exec_state.GetFrame(0).localName(0));
      assertEquals(void 0, exec_state.GetFrame(0).localValue(0).value());
      listenerCalled = true;
    }
  } catch (e) {
    exception = e;
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Call a function with local variables passing a different number parameters
// that the number of arguments.
(function(x,y){var a,b,c; debugger; return 3})()

// Make sure that the debug event listener vas invoked (again).
assertTrue(listenerCalled);
assertFalse(exception, "exception in listener")
