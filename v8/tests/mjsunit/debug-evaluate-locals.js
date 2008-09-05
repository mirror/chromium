// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerComplete = false;
exception = false;

function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      // Frame 0 has normal variables a and b.
      assertEquals('a', exec_state.GetFrame(0).localName(0));
      assertEquals('b', exec_state.GetFrame(0).localName(1));
      assertEquals(1, exec_state.GetFrame(0).localValue(0).value());
      assertEquals(2, exec_state.GetFrame(0).localValue(1).value());

      // Frame 1 has normal variable a (and the .arguments variable).
      assertEquals('.arguments', exec_state.GetFrame(1).localName(0));
      assertEquals('a', exec_state.GetFrame(1).localName(1));
      assertEquals(3, exec_state.GetFrame(1).localValue(1).value());

      // Frame 0 has normal variables a and b (and both the .arguments and
      // arguments variable).
      assertEquals('.arguments', exec_state.GetFrame(2).localName(0));
      assertEquals('a', exec_state.GetFrame(2).localName(1));
      assertEquals('arguments', exec_state.GetFrame(2).localName(2));
      assertEquals('b', exec_state.GetFrame(2).localName(3));
      assertEquals(5, exec_state.GetFrame(2).localValue(1).value());
      assertEquals(0, exec_state.GetFrame(2).localValue(3).value());

      // Evaluating a and b on frames 0, 1 and 2 produces 1, 2, 3, 4, 5 and 6.
      assertEquals(1, exec_state.GetFrame(0).evaluate('a').value());
      assertEquals(2, exec_state.GetFrame(0).evaluate('b').value());
      assertEquals(3, exec_state.GetFrame(1).evaluate('a').value());
      assertEquals(4, exec_state.GetFrame(1).evaluate('b').value());
      assertEquals(5, exec_state.GetFrame(2).evaluate('a').value());
      assertEquals(6, exec_state.GetFrame(2).evaluate('b').value());

      // Indicate that all was processed.
      listenerComplete = true;
    }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

function h() {
  var a = 1;
  var b = 2;
  debugger;  // Breakpoint.
};

function g() {
  var a = 3;
  eval("var b = 4;");
  h();
};

function f() {
  var a = 5;
  var b = 0;
  with ({b:6}) {
    g();
  }
};

f();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete);
assertFalse(exception, "exception in listener")
