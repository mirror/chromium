// Eval should be useable from eval.

if (42 == eval("eval('eval(\"42\")')")) print("PASSED");
else print("FAILED");
