// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple function which stores the last debug event.
listenerComplete = false;
exception = false;

var base_request = '"seq":0,"type":"request","command":"continue"'

function safeEval(code) {
  try {
    return eval('(' + code + ')');
  } catch (e) {
    assertEquals(void 0, e);
    return undefined;
  }
}

function testArguments(dcp, arguments, success) {
  // Generate request with the supplied arguments
  var request;
  if (arguments) {
    request = '{' + base_request + ',"arguments":' + arguments + '}';
  } else {
    request = '{' + base_request + '}'
  }
  var response = safeEval(dcp.processDebugJSONRequest(request));
  if (success) {
    assertTrue(response.success, request + ' -> ' + response.message);
    assertTrue(response.running, request + ' -> expected running');
  } else {
    assertFalse(response.success, request + ' -> ' + response.message);
    assertFalse(response.running, request + ' -> expected not running');
  }
}

function listener(event, exec_state, event_data, data) {
  try {
  if (event == Debug.DebugEvent.Break) {
    // Get the debug command processor.
    var dcp = exec_state.debugCommandProcessor();

    // Test simple continue request.
    testArguments(dcp, void 0, true);

    // Test some illegal continue requests.
    testArguments(dcp, '{"stepaction":"maybe"}', false);
    testArguments(dcp, '{"stepcount":-1}', false);

    // Test some legal continue requests.
    testArguments(dcp, '{"stepaction":"in"}', true);
    testArguments(dcp, '{"stepaction":"min"}', true);
    testArguments(dcp, '{"stepaction":"next"}', true);
    testArguments(dcp, '{"stepaction":"out"}', true);
    testArguments(dcp, '{"stepcount":1}', true);
    testArguments(dcp, '{"stepcount":10}', true);
    testArguments(dcp, '{"stepcount":"10"}', true);
    testArguments(dcp, '{"stepaction":"next","stepcount":10}', true);

    // Indicate that all was processed.
    listenerComplete = true;
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

function f() {
  a=1
};

function g() {
  f();
};

// Set a break point and call to invoke the debug event listener.
Debug.setBreakPoint(g, 0, 0);
g();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete, "listener did not run to completion");
assertFalse(exception, "exception in listener")
