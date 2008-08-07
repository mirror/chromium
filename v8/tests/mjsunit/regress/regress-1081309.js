// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Make sure that the backtrace command can be processed when the receiver is
// undefined.
listenerCalled = false;
exception = false;

function safeEval(code) {
  try {
    return eval('(' + code + ')');
  } catch (e) {
    return undefined;
  }
}

function listener(event, exec_state, event_data, data) {
  try {
  if (event == Debug.DebugEvent.Exception)
  {
    // The expected backtrace is
    // 1: g
    // 0: [anonymous]
    
    // Get the debug command processor.
    var dcp = exec_state.debugCommandProcessor();

    // Get the backtrace.
    var json;
    json = '{"seq":0,"type":"request","command":"backtrace"}'
    var backtrace = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(2, backtrace.totalFrames);
    assertEquals(2, backtrace.frames.length);

    assertEquals("g", backtrace.frames[0].func.name);
    assertEquals("", backtrace.frames[1].func.name);

    listenerCalled = true;
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Call method on undefined.
function g() {
  (void 0).f();
};

// Break on the exception to do a backtrace with undefined as receiver.
Debug.setBreakOnException(true);
try {
  g();
} catch(e) {
  // Ignore the exception "Cannot call method 'x' of undefined"
}

// Make sure that the debug event listener vas invoked.
assertTrue(listenerCalled, "listener not called");
assertFalse(exception, "exception in listener", exception)
