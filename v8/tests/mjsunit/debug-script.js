// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

Date();
RegExp();

// Count script types.
var native_count = 0;
var extension_count = 0;
var normal_count = 0;
var scripts = Debug.scripts();
for (i = 0; i < scripts.length; i++) {
  if (scripts[i].type == Debug.ScriptType.Native) {
    native_count++;
  } else if (scripts[i].type == Debug.ScriptType.Extension) {
    extension_count++;
  } else if (scripts[i].type == Debug.ScriptType.Normal) {
    if (!scripts[i].name) print("X" + scripts[i].source + "X"); // empty script
    else {
      print(scripts[i].name);
      normal_count++;
      }
  } else {
    assertUnreachable('Unexpected type ' + scripts[i].type);
  }
}

// This has to be updated if the number of native and extension scripts change.
assertEquals(12, native_count);
assertEquals(5, extension_count);
// TODO(sgjesse) This should be fixed to not include the empty script.
assertEquals(2, normal_count);  // This script and mjsunit.js.

// Test a builtins script.
var math_script = Debug.findScript('native math.js');
assertEquals('native math.js', math_script.name);
assertEquals(Debug.ScriptType.Native, math_script.type);

// Test a builtins delay loaded script.
var date_delay_script = Debug.findScript('native date.js');
assertEquals('native date.js', date_delay_script.name);
assertEquals(Debug.ScriptType.Native, date_delay_script.type);

// Test a debugger script.
var debug_delay_script = Debug.findScript('native debug.js');
assertEquals('native debug.js', debug_delay_script.name);
assertEquals(Debug.ScriptType.Native, debug_delay_script.type);

// Test an extension script.
var extension_gc_script = Debug.findScript('v8/gc');
assertEquals('v8/gc', extension_gc_script.name);
assertEquals(Debug.ScriptType.Extension, extension_gc_script.type);

// Test a normal script.
var mjsunit_js_script = Debug.findScript('mjsunit.js');
assertEquals('mjsunit.js', mjsunit_js_script.name);
assertEquals(Debug.ScriptType.Normal, mjsunit_js_script.type);

// Check a nonexistent script.
var dummy_script = Debug.findScript('dummy.js');
assertTrue(typeof dummy_script == 'undefined');
