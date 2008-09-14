// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// State to check that the listener code was invoked and that no exceptions
// occoured.
listenerComplete = false;
exception = false;

var base_request = '"seq":0,"type":"request","command":"scripts"'

function safeEval(code) {
  try {
    return eval('(' + code + ')');
  } catch (e) {
    assertEquals(void 0, e);
    return undefined;
  }
}

function testArguments(dcp, arguments, success) {
  var request = '{' + base_request + ',"arguments":' + arguments + '}'
  var json_response = dcp.processDebugJSONRequest(request);
  var response = safeEval(json_response);
  if (success) {
    assertTrue(response.success, json_response);
  } else {
    assertFalse(response.success, json_response);
  }
}

function listener(event, exec_state, event_data, data) {
  try {
  if (event == Debug.DebugEvent.Break) {
    // Get the debug command processor.
    var dcp = exec_state.debugCommandProcessor();

    // Test illegal scripts requests.
    testArguments(dcp, '{"types":"xx"}', false);

    // Test legal scripts requests.
    var request = '{' + base_request + '}'
    var response = safeEval(dcp.processDebugJSONRequest(request));
    assertTrue(response.success);
    testArguments(dcp, '{}', true);
    testArguments(dcp, '{"types":1}', true);
    testArguments(dcp, '{"types":2}', true);
    testArguments(dcp, '{"types":4}', true);
    testArguments(dcp, '{"types":7}', true);
    testArguments(dcp, '{"types":0xFF}', true);

    // Indicate that all was processed.
    listenerComplete = true;
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Call debugger to invoke the debug event listener.
debugger;

// Make sure that the debug event listener vas invoked with no exceptions.
assertTrue(listenerComplete, "listener did not run to completion");
assertFalse(exception, "exception in listener")
