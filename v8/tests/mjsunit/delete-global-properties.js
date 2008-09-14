// Global properties declared with 'var' or 'function' should not be
// deleteable.
var tmp;
assertFalse(delete tmp);  // should be DONT_DELETE
assertTrue("tmp" in this);
function f() { return 1; }
assertFalse(delete f);  // should be DONT_DELETE
assertEquals(1, f());  

/* Perhaps related to bugs/11? */
