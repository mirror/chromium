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

function listener(event, exec_state, event_data, data) {
  try {
  if (event == Debug.DebugEvent.Break) {
    // Get the debug command processor.
    var dcp = exec_state.debugCommandProcessor();

    // Test some illegal evaluate requests.
    testRequest(dcp, void 0, false);
    testRequest(dcp, '{"expression":"1","global"=true}', false);
    testRequest(dcp, '{"expression":"a","frame":4}', false);

    // Test some legal evaluate requests.
    testRequest(dcp, '{"expression":"1+2"}', true, 3);
    testRequest(dcp, '{"expression":"a+2"}', true, 5);
    testRequest(dcp, '{"expression":"({\\"a\\":1,\\"b\\":2}).b+2"}', true, 4);

    // Test evaluation of a in the stack frames and the global context.
    testRequest(dcp, '{"expression":"a"}', true, 3);
    testRequest(dcp, '{"expression":"a","frame":0}', true, 3);
    testRequest(dcp, '{"expression":"a","frame":1}', true, 2);
    testRequest(dcp, '{"expression":"a","frame":2}', true, 1);
    testRequest(dcp, '{"expression":"a","global":true}', true, 1);
    testRequest(dcp, '{"expression":"this.a","global":true}', true, 1);

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
  var a = 3;
};

function g() {
  var a = 2;
  f();
};

a = 1;

// Set a break point at return in f and invoke g to hit the breakpoint.
Debug.setBreakPoint(f, 2, 0);
g();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerComplete, "listener did not run to completion");
assertFalse(exception, "exception in listener")
