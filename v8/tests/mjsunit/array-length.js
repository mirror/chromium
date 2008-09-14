var a = [0,1,2,3];
a.length = 0;

assertEquals('undefined', typeof a[0]);
assertEquals('undefined', typeof a[1]);
assertEquals('undefined', typeof a[2]);
assertEquals('undefined', typeof a[3]);


var a = [0,1,2,3];
a.length = 2;

assertEquals(0, a[0]);
assertEquals(1, a[1]);
assertEquals('undefined', typeof a[2]);
assertEquals('undefined', typeof a[3]);


var a = new Array();
a[0] = 0;
a[1000] = 1000;
a[1000000] = 1000000;
a[2000000] = 2000000;

assertEquals(2000001, a.length);
a.length = 0;
assertEquals(0, a.length);
assertEquals('undefined', typeof a[0]);
assertEquals('undefined', typeof a[1000]);
assertEquals('undefined', typeof a[1000000]);
assertEquals('undefined', typeof a[2000000]);


var a = new Array();
a[0] = 0;
a[1000] = 1000;
a[1000000] = 1000000;
a[2000000] = 2000000;

assertEquals(2000001, a.length);
a.length = 2000;
assertEquals(2000, a.length);
assertEquals(0, a[0]);
assertEquals(1000, a[1000]);
assertEquals('undefined', typeof a[1000000]);
assertEquals('undefined', typeof a[2000000]);


var a = new Array();
a[Math.pow(2,31)-1] = 0;
a[Math.pow(2,30)-1] = 0;
assertEquals(Math.pow(2,31), a.length);


var a = new Array();
a[0] = 0;
a[1000] = 1000;
a[Math.pow(2,30)-1] = Math.pow(2,30)-1;
a[Math.pow(2,31)-1] = Math.pow(2,31)-1;
a[Math.pow(2,32)-2] = Math.pow(2,32)-2;

assertEquals(Math.pow(2,30)-1, a[Math.pow(2,30)-1]);
assertEquals(Math.pow(2,31)-1, a[Math.pow(2,31)-1]);
assertEquals(Math.pow(2,32)-2, a[Math.pow(2,32)-2]);

assertEquals(Math.pow(2,32)-1, a.length);
a.length = Math.pow(2,30)+1;  // not a smi!
assertEquals(Math.pow(2,30)+1, a.length);

assertEquals(0, a[0]);
assertEquals(1000, a[1000]);
assertEquals(Math.pow(2,30)-1, a[Math.pow(2,30)-1]);
assertEquals('undefined', typeof a[Math.pow(2,31)-1]);
assertEquals('undefined', typeof a[Math.pow(2,32)-2], "top");


var a = new Array();
a.length = new Number(12);
assertEquals(12, a.length);


var o = { length: -23 };
Array.prototype.pop.apply(o);
assertEquals(4294967272, o.length);
