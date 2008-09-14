// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Simple debug event handler which counts the number of breaks hit and steps.
var break_break_point_hit_count = 0;
function listener(event, exec_state, event_data, data) {
  if (event == Debug.DebugEvent.Break) {
    break_break_point_hit_count++;
    // Continue stepping until returned to bottom frame.
    if (exec_state.GetFrameCount() > 1) {
      exec_state.prepareStep(Debug.StepAction.StepIn);
    }
  }
};

// Add the debug event listener.
Debug.addListener(listener);

// Use 'eval' to ensure that the call to print is through CodeStub CallFunction.
// See Ia32CodeGenerator::VisitCall and Ia32CodeGenerator::CallWithArguments.
function f() {
  debugger;
  eval('');
  print('Hello, world!');
};

break_break_point_hit_count = 0;
f();
assertEquals(5, break_break_point_hit_count);

// Use an inner function to ensure that the function call is through CodeStub
// CallFunction see Ia32CodeGenerator::VisitCall and
// Ia32CodeGenerator::CallWithArguments.
function g() {
  function h() {}
  debugger;
  h();
};

break_break_point_hit_count = 0;
g();
assertEquals(4, break_break_point_hit_count);

// Get rid of the debug event listener.
Debug.removeListener(listener);
