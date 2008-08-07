// Correctly causes a stack overflow, but the VM shouldn't crash
// with "Segmentation fault" but instead provide a decent error message.

function h() {
  h();
}

try {
  h();
} catch (e) {
  print("Caught stack overflow exception.");
  print(e);
}


// Try to get a stack overflow exception thrown from the parser.
try {
  var s = "eval(s)";
  eval(s);
} catch (e) {
  print("Caught stack overflow exception from parser.");
  print(e);
}
