args = "arg0";
f = new Function( args, "var r=0; var i = 0; { if ( eval('arg'+i) == void 0) break; } return r;");

print(eval("f('')"))
