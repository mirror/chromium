// Flags: --expose-debug-as debug
// The functions used for testing backtraces. They are at the top to make the
// testing of source line/column easier.
function f(x, y) {
  a=1;
};

function g() {
  new f(1);
};


// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

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
  if (event == Debug.DebugEvent.Break)
  {
    // The expected backtrace is
    // 0: f
    // 1: g
    // 2: [anonymous]
    
    // Get the debug command processor.
    var dcp = exec_state.debugCommandProcessor();

    // Get the backtrace.
    var json;
    json = '{"seq":0,"type":"request","command":"backtrace"}'
    var backtrace = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(0, backtrace.fromFrame);
    assertEquals(3, backtrace.toFrame);
    assertEquals(3, backtrace.totalFrames);
    var frames = backtrace.frames;
    assertEquals(3, frames.length);
    for (var i = 0; i < frames.length; i++) {
      assertEquals('frame', frames[i].type);
    }
    assertEquals(0, frames[0].index);
    assertEquals("f", frames[0].func.name);
    assertEquals(1, frames[1].index);
    assertEquals("g", frames[1].func.name);
    assertEquals(2, frames[2].index);
    assertEquals("", frames[2].func.name);

    // Get backtrace with two frames.
    json = '{"seq":0,"type":"request","command":"backtrace","arguments":{"fromFrame":1,"toFrame":3}}'
    var backtrace = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(1, backtrace.fromFrame);
    assertEquals(3, backtrace.toFrame);
    assertEquals(3, backtrace.totalFrames);
    var frames = backtrace.frames;
    assertEquals(2, frames.length);
    for (var i = 0; i < frames.length; i++) {
      assertEquals('frame', frames[i].type);
    }
    assertEquals(1, frames[0].index);
    assertEquals("g", frames[0].func.name);
    assertEquals(2, frames[1].index);
    assertEquals("", frames[1].func.name);

    // Get the individual frames.
    var frame;
    json = '{"seq":0,"type":"request","command":"frame"}'
    frame = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(0, frame.index);
    assertEquals("f", frame.func.name);
    assertTrue(frame.constructCall);
    assertEquals(4, frame.line);
    assertEquals(3, frame.column);
    assertEquals(2, frame.arguments.length);
    assertEquals('x', frame.arguments[0].name);
    assertEquals('number', frame.arguments[0].value.type);
    assertEquals(1, frame.arguments[0].value.value);
    assertEquals('y', frame.arguments[1].name);
    assertEquals('undefined', frame.arguments[1].value.type);

    json = '{"seq":0,"type":"request","command":"frame","arguments":{"number":0}}'
    frame = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(0, frame.index);
    assertEquals("f", frame.func.name);
    assertEquals(4, frame.line);
    assertEquals(3, frame.column);
    assertEquals(2, frame.arguments.length);
    assertEquals('x', frame.arguments[0].name);
    assertEquals('number', frame.arguments[0].value.type);
    assertEquals(1, frame.arguments[0].value.value);
    assertEquals('y', frame.arguments[1].name);
    assertEquals('undefined', frame.arguments[1].value.type);

    json = '{"seq":0,"type":"request","command":"frame","arguments":{"number":1}}'
    frame = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(1, frame.index);
    assertEquals("g", frame.func.name);
    assertFalse(frame.constructCall);
    assertEquals(8, frame.line);
    assertEquals(2, frame.column);
    assertEquals(0, frame.arguments.length);

    json = '{"seq":0,"type":"request","command":"frame","arguments":{"number":2}}'
    frame = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(2, frame.index);
    assertEquals("", frame.func.name);

    // Source slices for the individual frames (they all refer to this script).
    json = '{"seq":0,"type":"request","command":"source",' +
            '"arguments":{"frame":0,"fromLine":3,"toLine":5}}'
    source = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals("function f(x, y) {", source.source.substring(0, 18));
    assertEquals(3, source.fromLine);
    assertEquals(5, source.toLine);
    
    json = '{"seq":0,"type":"request","command":"source",' +
            '"arguments":{"frame":1,"fromLine":4,"toLine":5}}'
    source = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals("  a=1;", source.source.substring(0, 6));
    assertEquals(4, source.fromLine);
    assertEquals(5, source.toLine);
    
    json = '{"seq":0,"type":"request","command":"source",' +
            '"arguments":{"frame":2,"fromLine":8,"toLine":9}}'
    source = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals("  new f(1);", source.source.substring(0, 11));
    assertEquals(8, source.fromLine);
    assertEquals(9, source.toLine);
    
    // Test line interval way beyond this script will result in an error.
    json = '{"seq":0,"type":"request","command":"source",' +
            '"arguments":{"frame":0,"fromLine":10000,"toLine":20000}}'
    response = safeEval(dcp.processDebugJSONRequest(json));
    assertFalse(response.success);
    
    // Test without arguments.
    json = '{"seq":0,"type":"request","command":"source"}'
    source = safeEval(dcp.processDebugJSONRequest(json)).body;
    assertEquals(Debug.findScript(f).source, source.source);
    
    listenerCalled = true;
  }
  } catch (e) {
    exception = e
  };
};

// Add the debug event listener.
Debug.addListener(listener);

// Set a break point and call to invoke the debug event listener.
Debug.setBreakPoint(f, 0, 0);
g();

// Make sure that the debug event listener vas invoked.
assertTrue(listenerCalled);
assertFalse(exception, "exception in listener");
