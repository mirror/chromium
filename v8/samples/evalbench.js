/* This is the starting point for an eval() benchmark. */

var start = new Date();
for (eval("var i = 0"); eval("i < 10000"); eval("i++")) ;
print("Time: " + (new Date() - start) + " ms.");
