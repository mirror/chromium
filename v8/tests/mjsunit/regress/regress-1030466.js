// Whenever we enter a with-scope, we copy the context. This in itself is fine
// (contexts may escape), but when leaving a with-scope, we currently also copy
// the context instead of reverting to the original. This does not work because
// inner functions may already have been created using the original context. In
// the failing test case below, the inner function is run in the original context
// (where x is undefined), but the assignment to x after the with-statement is
// run in the copied context:

var result = (function outer() {
 with ({}) { }
 var x = 10;
 function inner() {
   return x;
 };
 return inner();
})();

assertEquals(10, result);
