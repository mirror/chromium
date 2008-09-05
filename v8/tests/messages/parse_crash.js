// ecma/LexicalConventions/7.3-13-n.js
// ecma/LexicalConventions/7.3-13-n.js
// ecma/LexicalConventions/7.4.1-1-n.js
// ecma/LexicalConventions/7.4.1-2-n.js
// ecma/LexicalConventions/7.4.1-3-n.js
// ecma/LexicalConventions/7.4.2-1-n.js
// ecma/LexicalConventions/7.4.2-10-n.js
// ecma/LexicalConventions/7.4.2-11-n.js
// ecma/LexicalConventions/7.4.2-12-n.js
// ecma/LexicalConventions/7.4.2-13-n.js
// ecma/LexicalConventions/7.4.2-14-n.js
// ecma/LexicalConventions/7.4.2-15-n.js
// ecma/LexicalConventions/7.4.2-16-n.js
// ecma/LexicalConventions/7.4.2-2-n.js
// ....
//
// Crashes the VM in parser

print(eval("/*/*\"fail\";*/*/"))