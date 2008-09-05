// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerCalled = false;

function listener(event, exec_state, event_data, data) {
 listenerCalled = true;
 throw 1;
};

// Add the debug event listener.
Debug.addListener(listener);

function f() {
 a=1
};

// Set a break point and call to invoke the debug event listener.
Debug.setBreakPoint(f, 0, 0);
f();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerCalled);