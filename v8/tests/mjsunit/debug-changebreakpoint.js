// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple function which stores the last debug event.
listenerComplete = false;
exception = false;

var base_request = '"seq":0,"type":"request","command":"changebreakpoint"'

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

    // Test some illegal clearbreakpoint requests.
    var request = '{' + base_request + '}'
    var response = safeEval(dcp.processDebugJSONRequest(request));
    assertFalse(response.success);

    testArguments(dcp, '{}', false);
    testArguments(dcp, '{"breakpoint":0,"condition":"false"}', false);
    // TODO(1241036) change this to 2 when break points have been restructured.
    testArguments(dcp, '{"breakpoint":3,"condition":"false"}', false);
    testArguments(dcp, '{"breakpoint":"xx","condition":"false"}', false);

    // Test some legal clearbreakpoint requests.
    testArguments(dcp, '{"breakpoint":1}', true);
    testArguments(dcp, '{"breakpoint":1,"enabled":"true"}', true);
    testArguments(dcp, '{"breakpoint":1,"enabled":"false"}', true);
    testArguments(dcp, '{"breakpoint":1,"condition":"1==2"}', true);
    testArguments(dcp, '{"breakpoint":1,"condition":"false"}', true);
    testArguments(dcp, '{"breakpoint":1,"ignoreCount":7}', true);
    testArguments(dcp, '{"breakpoint":1,"ignoreCount":0}', true);
    testArguments(
        dcp,
        '{"breakpoint":1,"enabled":"true","condition":"false","ignoreCount":0}',
        true);

    // Indicate that all was processed.
    listenerComplete = true;
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

function g() {};

// Set a break point and call to invoke the debug event listener.
bp = Debug.setBreakPoint(g, 0, 0);
assertEquals(1, bp);
g();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete, "listener did not run to completion");
assertFalse(exception, "exception in listener")
