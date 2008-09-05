// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

listenerComplete = false;
exception = false;

// The base part of all evaluate requests.
var base_request = '"seq":0,"type":"request","command":"evaluate"'

function safeEval(code) {
  try {
    return eval('(' + code + ')');
  } catch (e) {
    assertEquals(void 0, e);
    return undefined;
  }
}

function testRequest(dcp, arguments, success, result) {
  // Generate request with the supplied arguments.
  var request;
  if (arguments) {
    request = '{' + base_request + ',"arguments":' + arguments + '}';
  } else {
    request = '{' + base_request + '}'
  }
  var response = safeEval(dcp.processDebugJSONRequest(request));
  if (success) {
    assertTrue(response.success, request + ' -> ' + response.message);
    assertEquals(result, response.body.value);
  } else {
    assertFalse(response.success, request + ' -> ' + response.message);
  }
  assertFalse(response.running, request + ' -> expected not running');
}


// Event listener which evaluates with break disabled.
function listener(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      // Call functions with break using the FrameMirror directly.
      assertEquals(1, exec_state.GetFrame(0).evaluate('f()', true).value());
      assertEquals(2, exec_state.GetFrame(0).evaluate('g()', true).value());

      // Get the debug command processor.
      var dcp = exec_state.debugCommandProcessor();

      // Call functions with break using the JSON protocol. Tests that argument
      // disable_break is default true.
      testRequest(dcp, '{"expression":"f()"}', true, 1);
      testRequest(dcp, '{"expression":"f()","frame":0}',  true, 1);
      testRequest(dcp, '{"expression":"g()"}', true, 2);
      testRequest(dcp, '{"expression":"g()","frame":0}',  true, 2);

      // Call functions with break using the JSON protocol. Tests passing
      // argument disable_break is default true.
      testRequest(dcp, '{"expression":"f()","disable_break":true}', true, 1);
      testRequest(dcp, '{"expression":"f()","frame":0,"disable_break":true}',
                  true, 1);
      testRequest(dcp, '{"expression":"g()","disable_break":true}', true, 2);
      testRequest(dcp, '{"expression":"g()","frame":0,"disable_break":true}',
                  true, 2);

      // Indicate that all was processed.
      listenerComplete = true;
    }
  } catch (e) {
    exception = e
  };
};


// Event listener which evaluates with break enabled one time and the second
// time evaluates with break disabled.
var break_count = 0;
function listener_recurse(event, exec_state, event_data, data) {
  try {
    if (event == Debug.DebugEvent.Break)
    {
      break_count++;
      
      // Call functions with break using the FrameMirror directly.
      if (break_count == 1) {
        // First break event evaluates with break enabled.
        assertEquals(1, exec_state.GetFrame(0).evaluate('f()', false).value());
        listenerComplete = true;
      } else {
        // Second break event evaluates with break disabled.
        assertEquals(2, break_count);
        assertFalse(listenerComplete);
        assertEquals(1, exec_state.GetFrame(0).evaluate('f()', true).value());
      }
    }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Test functions - one with break point and one with debugger statement.
function f() {
  return 1;
};

function g() {
  debugger;
  return 2;
};

Debug.setBreakPoint(f, 2, 0);

// Cause a debug break event.
debugger;

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete);
assertFalse(exception, "exception in listener")

// Remove the debug event listener.
Debug.removeListener(listener);

// Add debug event listener wich uses recursive breaks.
listenerComplete = false;
Debug.addListener(listener_recurse);

debugger;

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete);
assertFalse(exception, "exception in listener")
assertEquals(2, break_count);
