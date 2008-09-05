// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

// Set and remove a script break point.
var sbp = Debug.setScriptBreakPoint("1", 2, 3);
assertEquals(1, Debug.scriptBreakPoints().length);
assertEquals("1", Debug.scriptBreakPoints()[0].script_name());
assertEquals(2, Debug.scriptBreakPoints()[0].line());
assertEquals(3, Debug.scriptBreakPoints()[0].column());
Debug.clearBreakPoint(sbp);
assertEquals(0, Debug.scriptBreakPoints().length);

// Set three script break points.
var sbp1 = Debug.setScriptBreakPoint("1", 2, 3);
var sbp2 = Debug.setScriptBreakPoint("2", 3, 4);
var sbp3 = Debug.setScriptBreakPoint("3", 4, 5);

// Check the content of the script break points.
assertEquals(3, Debug.scriptBreakPoints().length);
for (var i = 0; i < Debug.scriptBreakPoints().length; i++) {
  var x = Debug.scriptBreakPoints()[i];
  if ("1" == x.script_name()) {
    assertEquals(2, x.line());
    assertEquals(3, x.column());
  } else if ("2" == x.script_name()) {
    assertEquals(3, x.line());
    assertEquals(4, x.column());
  } else if ("3" == x.script_name()) {
    assertEquals(4, x.line());
    assertEquals(5, x.column());
  } else {
    assertUnreachable("unecpected script_data " + x.script_data());
  }
}

// Remove script break points (in another order than they where added).
assertEquals(3, Debug.scriptBreakPoints().length);
Debug.clearBreakPoint(sbp1);
assertEquals(2, Debug.scriptBreakPoints().length);
Debug.clearBreakPoint(sbp3);
assertEquals(1, Debug.scriptBreakPoints().length);
Debug.clearBreakPoint(sbp2);
assertEquals(0, Debug.scriptBreakPoints().length);
