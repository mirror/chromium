// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple function which stores the last debug event.
lastDebugEvent = new Object();
function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break ||
      event == Debug.DebugEvent.Exception)
  {
    lastDebugEvent.event = event;
    lastDebugEvent.frameFuncName = exec_state.GetFrame().func().name();
    lastDebugEvent.event_data = event_data;
  }
};

// Add the debug event listener.
Debug.addListener(listener);
// Get events from handled exceptions.
Debug.setBreakOnException();

// Test debug event for handled exception.
(function f(){
  try {
    x();
  } catch(e) {
    // Do nothing. Ignore exception.
  }
})();
assertTrue(lastDebugEvent.event == Debug.DebugEvent.Exception);
assertEquals(lastDebugEvent.frameFuncName, "f");
assertFalse(lastDebugEvent.event_data.uncaught());
Debug.clearBreakOnException();

// Test debug event for break point.
function a() {
  x = 1;
  y = 2;
  z = 3;
};
Debug.setBreakPoint(a, 1);
a();
assertTrue(lastDebugEvent.event == Debug.DebugEvent.Break);
assertEquals(lastDebugEvent.frameFuncName, "a");

Debug.removeListener(listener);
