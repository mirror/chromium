// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerComplete = false;
exception = false;

function checkArguments(frame, names, values) {
  var argc = Math.max(names.length, values.length);
  assertEquals(argc, frame.argumentCount());
  for (var i = 0; i < argc; i++) {
    if (i < names.length) {
      assertEquals(names[i], frame.argumentName(i));
    } else {
      assertEquals(void 0, frame.argumentName(i));
    }

    if (i < values.length) {
      assertEquals(values[i], frame.argumentValue(i).value());
    } else {
      assertEquals(void 0, frame.argumentValue(i).value());
    }
  }
}

function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      // Frame 0 - called with less parameters than arguments.
      checkArguments(exec_state.GetFrame(0), ['x', 'y'], [1]);

      // Frame 1 - called with more parameters than arguments.
      checkArguments(exec_state.GetFrame(1), ['x', 'y'], [1, 2, 3]);

      // Frame 2 - called with same number of parameters than arguments.
      checkArguments(exec_state.GetFrame(2), ['x', 'y', 'z'], [1, 2, 3]);

      // Indicate that all was processed.
      listenerComplete = true;
    }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

function h(x, y) {
  debugger;  // Breakpoint.
};

function g(x, y) {
  h(x);
};

function f(x, y, z) {
  g.apply(null, [x, y, z]);
};

f(1, 2, 3);

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete);
assertFalse(exception, "exception in listener")
