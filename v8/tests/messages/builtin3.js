// See if we can't access natives by tricking the parser.
function foo() {
  x = 0 %StringEquals('a', 'b');
  return x;
}


foo();
