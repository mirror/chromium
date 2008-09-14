// Test multiline string literal.
var str = 'asdf\
\nasdf\
\rasdf\
\tasdf\
\\\
\
';
assertEquals('asdf\nasdf\rasdf\tasdf\\', str);

// Allow CR+LF in multiline string literals.
var code = "'asdf\\" + String.fromCharCode(0xD) + String.fromCharCode(0xA) + "asdf'";
assertEquals('asdfasdf', eval(code));

// Allow LF+CR in multiline string literals.
code = "'asdf\\" + String.fromCharCode(0xA) + String.fromCharCode(0xD) + "asdf'";
assertEquals('asdfasdf', eval(code));


