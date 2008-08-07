// From: http://wiki.ecmascript.org/doku.php?id=es3.1:catch_clause_context_specification
// For more info on this see http://b/issue?id=1178598 .


function foo() {
  this.x = 11;
}
 
x = "global.x";

try {
  throw foo;
} catch (e) {
  print(x) // Should print "global.x"
  e();     // Should add x to e (both IE and Firefox modify the global x)
  print(x) // Should print 11
}

print(x);  // Should print "global.x". IE and Firefox both print 11
