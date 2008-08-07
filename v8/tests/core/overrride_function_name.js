// Failure from: ecma/ExecutionContexts/10.1.3.js
// (overriding a function name with a variable_
//
// Crash in DEBUG mode in ICS::Initialize.
//
// #
// # Fatal error in /home/srdjan/v8/src/objects-inl.h, line 483
// # CHECK(object->IsJSFunction()) failed
// #

function t() { 
  return 1
}

var t = 2; 

if (t != 2)
  print("error: t = " + t + ", expected 2");
else
  print("passed");