// Test date construction from other dates.
var date0 = new Date(1111);
var date1 = new Date(date0);
assertEquals(1111, date0.getTime());
assertEquals(date0.getTime(), date1.getTime());
var date2 = new Date(date0.toString());
assertEquals(1000, date2.getTime());

// Test that dates may contain commas.
var date0 = Date.parse("Dec 25 1995 1:30");
var date1 = Date.parse("Dec 25, 1995 1:30");
var date2 = Date.parse("Dec 25 1995, 1:30");
var date3 = Date.parse("Dec 25, 1995, 1:30");
assertEquals(date0, date1);
assertEquals(date1, date2);
assertEquals(date2, date3);


// Tests inspired by js1_5/Date/regress-346363.js

// Year
var a = new Date();
a.setFullYear();
a.setFullYear(2006);
assertEquals(2006, a.getFullYear());

var b = new Date();
b.setUTCFullYear();
b.setUTCFullYear(2006);
assertEquals(2006, b.getUTCFullYear());

// Month
var c = new Date();
c.setMonth();
c.setMonth(2);
assertTrue(isNaN(c.getMonth()));

var d = new Date();
d.setUTCMonth();
d.setUTCMonth(2);
assertTrue(isNaN(d.getUTCMonth()));

// Date
var e = new Date();
e.setDate();
e.setDate(2);
assertTrue(isNaN(e.getDate()));

var f = new Date();
f.setUTCDate();
f.setUTCDate(2);
assertTrue(isNaN(f.getUTCDate()));

// Hours
var g = new Date();
g.setHours();
g.setHours(2);
assertTrue(isNaN(g.getHours()));

var h = new Date();
h.setUTCHours();
h.setUTCHours(2);
assertTrue(isNaN(h.getUTCHours()));

// Minutes
var g = new Date();
g.setMinutes();
g.setMinutes(2);
assertTrue(isNaN(g.getMinutes()));

var h = new Date();
h.setUTCHours();
h.setUTCHours(2);
assertTrue(isNaN(h.getUTCHours()));


// Seconds
var i = new Date();
i.setSeconds();
i.setSeconds(2);
assertTrue(isNaN(i.getSeconds()));

var j = new Date();
j.setUTCSeconds();
j.setUTCSeconds(2);
assertTrue(isNaN(j.getUTCSeconds()));


// Milliseconds
var k = new Date();
k.setMilliseconds();
k.setMilliseconds(2);
assertTrue(isNaN(k.getMilliseconds()));

var l = new Date();
l.setUTCMilliseconds();
l.setUTCMilliseconds(2);
assertTrue(isNaN(l.getUTCMilliseconds()));

