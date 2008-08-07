--> must work at beginning of file!

var x = 1;
--> this must be ignored...
   --> so must this...
	--> and this.
x-->0;
assertEquals(0, x);


var x = 0; x <!-- x
assertEquals(0, x);

var x = 1; x <!--x
assertEquals(1, x);

var x = 2; x <!-- x; x = 42;
assertEquals(2, x);

var x = 1; x <! x--;
assertEquals(0, x);

var x = 1; x <!- x--;
assertEquals(0, x);

var b = true <! true;
assertFalse(b);

var b = true <!- true;
assertFalse(b);
