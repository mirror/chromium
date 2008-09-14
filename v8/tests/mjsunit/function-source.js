// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Check that the script source for all functions in a script is the same.
function f() {
  function h() {
    assertEquals(Debug.scriptSource(f), Debug.scriptSource(h));
  }
  h();
}
  
function g() {
  function h() {
    assertEquals(Debug.scriptSource(f), Debug.scriptSource(h));
  }
  h();
}

assertEquals(Debug.scriptSource(f), Debug.scriptSource(g));
f();
g();

assertEquals("function print() { [native code] }", print);
