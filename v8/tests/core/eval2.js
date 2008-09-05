// Incorrect result of 'eval' for loops
//
// Cause of failure in:
//
// ecma//Expressions/11.4.1.js
// ecma/FunctionObjects/15.3.1.1-3.js

var num_failures = 0

var t = eval('for(i=1; i <3; ++i) { x = i }')
if (t != 2) {
  print("ERROR: bad eval result; should be 2, got " + t)
  ++num_failures
}

t = eval("var r = 0; for (var i = 0; i < 10; i++) { r += 1 }")
if (t != 10) {
  print("ERROR: bad eval result; should be 10, got " + t)
  ++num_failures
}

// ecma/Statements/12.5-1.js

t = eval("if ( true ) MYVAR='PASSED'; else MYVAR= 'FAILED';")
if (t != 'PASSED') {
  print("ERROR: bad eval result; should be 'PASSED', got " + t)
  ++num_failures
}

print(num_failures + " failures.")
