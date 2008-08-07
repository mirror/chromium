// ------------------------------------------------------
// [ecma/String/15.5.4.4-4.js]

Boolean.prototype.charAt = String.prototype.charAt;
var x = false;
assertEquals("f", x.charAt(0));
assertEquals("a", x.charAt(1));
assertEquals("l", x.charAt(2));
assertEquals("s", x.charAt(3));
assertEquals("e", x.charAt(4));
x = true;
assertEquals("t", x.charAt(0));
assertEquals("r", x.charAt(1));
assertEquals("u", x.charAt(2));
assertEquals("e", x.charAt(3));

Number.prototype.charAt = String.prototype.charAt;
x = NaN;
assertEquals("N", x.charAt(0));
assertEquals("a", x.charAt(1));
assertEquals("N", x.charAt(2));
x = 123;
assertEquals("1", x.charAt(0));
assertEquals("2", x.charAt(1));
assertEquals("3", x.charAt(2));
x = new Number(123);
assertEquals("1", x.charAt(0));
assertEquals("2", x.charAt(1));
assertEquals("3", x.charAt(2));

Array.prototype.charAt = String.prototype.charAt;
x = new Array(1,2,3);
assertEquals("1", x.charAt(0));
assertEquals(",", x.charAt(1));
assertEquals("2", x.charAt(2));
assertEquals(",", x.charAt(3));
assertEquals("3", x.charAt(4));
x = new Array();
assertEquals("", x.charAt(0));

x = new Object();
x.charAt = String.prototype.charAt;
assertEquals("[", x.charAt(0));
assertEquals("o", x.charAt(1));
assertEquals("b", x.charAt(2));
assertEquals("j", x.charAt(3));
assertEquals("e", x.charAt(4));
assertEquals("c", x.charAt(5));
assertEquals("t", x.charAt(6));
assertEquals(" ", x.charAt(7));
assertEquals("O", x.charAt(8));
assertEquals("b", x.charAt(9));
assertEquals("j", x.charAt(10));
assertEquals("e", x.charAt(11));
assertEquals("c", x.charAt(12));
assertEquals("t", x.charAt(13));
assertEquals("]", x.charAt(14));


// TODO(ager): add these tests when the function constructor is implemented.
//
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)",            "[",    eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(0)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(1)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)",            "b",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(2)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)",            "j",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(3)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)",            "e",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(4)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(5)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(6)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)",            " ",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(7)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)",            "F",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(8)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)",            "u",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(9)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(10)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)",            "c",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(11)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)",            "t",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(12)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)",            "i",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(13)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)",            "o",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(14)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)",            "n",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(15)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)",            "]",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(16)") );
// new TestCase( SECTION,     "x = new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)",            "",     eval("x=new Function(); x.toString = Object.prototype.toString; x.charAt = String.prototype.charAt; x.charAt(17)") );

// ------------------------------------------------------
// [ecma/String/15.5.4.5-6.js]

Boolean.prototype.charCodeAt = String.prototype.charCodeAt;
var obj = true;
var s = '';
for (var i = 0; i < 4; i++) s += String.fromCharCode(obj.charCodeAt(i));
assertEquals("true", s);

Number.prototype.charCodeAt = String.prototype.charCodeAt
var obj = 1234;
var s = '';
for (var i = 0; i < 4; i++) s += String.fromCharCode(obj.charCodeAt(i));
assertEquals("1234", s);

