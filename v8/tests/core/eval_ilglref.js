// ecma/ExecutionContexts/10.1.3.js
// 
// Should return 1; V8 throws exception Uncaught TypeError: Illegal reference to 't2'.

var r = eval("function t2(a,b) { this.a = b; this.b = a; }; x = new t2(1,3); x.b;");
print(r);
