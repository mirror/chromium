// Flags: --expose-debug-as debug
// Source lines for test.
var lines = [ 'function a() { b(); };\n',
              'function    b() {\n',
              '  c(true);\n',
              '};\n',
              '  function c(x) {\n',
              '    if (x) {\n',
              '      return 1;\n',
              '    } else {\n',
              '      return 1;\n',
              '    }\n',
              '  };\n' ];

// Build source by putting all lines together
var source = '';
for (var i = 0; i < lines.length; i++) {
  source += lines[i];
}
eval(source);

// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Get the script object from one of the functions in the source.
var script = Debug.findScript(a);

// Make sure that the source is as expected.
assertEquals(source, script.source);
assertEquals(source, script.sourceSlice().sourceText());

// Try all possible line interval slices.
for (var slice_size = 0; slice_size < lines.length; slice_size++) {
  for (var n = 0; n < lines.length - slice_size; n++) {
    var slice = script.sourceSlice(n, n + slice_size);
    assertEquals(n, slice.from_line);
    assertEquals(n + slice_size, slice.to_line);

    var text = slice.sourceText();
    var expected = '';
    for (var i = 0; i < slice_size; i++) {
      expected += lines[n + i];
    }
    assertEquals(expected, text);
  }
}
