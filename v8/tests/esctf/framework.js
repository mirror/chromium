var $PRINT = print;

function $FAIL(message) {
  print("Fail: " + message);
}

function $ERROR(message) {
  print("Error: " + message);
}

function $QUIT() {
  quit();
}
