// If the syntax error is in a try/catch block, it should not be printed
// out to the screen like an uncaught exception.

function shouldThrow(_a, _e)
{
  var exception;
  var _av;
  try {
    eval(_a);
  } catch (e) {
    exception = e;
  }

  var _ev;
  if (_e)
     _ev = eval(_e);

  if (exception) {
    if (typeof _e == "undefined" || exception == _ev)
      print("PASS: " + _a + " threw exception " + exception + ".");
    else
      print("FAIL: " + _a + " should throw exception " +_ev + ". Threw exception " + exception + ".");
  } else if (typeof _av == "undefined") {
    print("FAIL: " + _a + " should throw exception " + _e + ". Was undefined.");
  } else {
    print("FAIL: " + _a + " should throw exception " + _e + ". Was " + _av + ".");
  }
}

var x = 0;
var y = 0;

shouldThrow('(y, x) = "FAIL";');
