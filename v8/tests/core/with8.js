// The following code caused problems for the new context code
// because it pushed the global object as the 'with' object.
// As a result the Context::is_global_context() test failed
// which caused a runtime crash.

with (this) {
  eval("function (){}");
}

with (this) {
  (function (){});
}

with (this) {
  with (this) {
    eval("function (){}");
  }
}

with (this) {
  with (this) {
    (function (){});
  }
}
