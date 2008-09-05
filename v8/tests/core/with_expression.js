// using with expression gives an exception.
try {
  with(null) {}
} catch (e) {
  print(e);
}

// test exception throwing while evaluating the with expression. 
function foo() {
  throw 'Exception in with expression';
}

try {
  with(foo()) {}
} catch (e) {
  print(e);
}


