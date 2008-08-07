// All variables must be declared upon entering eval() code;
// even if they are nested inside statements and the statement
// body is not executed.

function f(s) {
  eval(s);
}

f("print(x); if (false) { var x = 5; }");  // undefined

