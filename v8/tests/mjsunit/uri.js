// Tests of URI encoding and decoding.

assertEquals("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_.!~*'();/?:@&=+$,#",
             encodeURI("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_.!~*'();/?:@&=+$,#"));

var cc1 = 0x007D;
var s1 = String.fromCharCode(cc1);
var cc2 = 0x0000;
var s2 = String.fromCharCode(cc2);
var cc3 = 0x0080;
var s3 = String.fromCharCode(cc3);
var cc4 = 0x0555;
var s4 = String.fromCharCode(cc4);
var cc5 = 0x07FF;
var s5 = String.fromCharCode(cc5);
var cc6 = 0x0800;
var s6 = String.fromCharCode(cc6);
var cc7 = 0xAEEE;
var s7 = String.fromCharCode(cc7);
var cc8_1 = 0xD800;
var cc8_2 = 0xDC00;
var s8 = String.fromCharCode(cc8_1)+String.fromCharCode(cc8_2);
var cc9_1 = 0xDBFF;
var cc9_2 = 0xDFFF;
var s9 = String.fromCharCode(cc9_1)+String.fromCharCode(cc9_2);
var cc10 = 0xE000;
var s10 = String.fromCharCode(cc10);

assertEquals('%7D', encodeURI(s1));
assertEquals('%00', encodeURI(s2));
assertEquals('%C2%80', encodeURI(s3));
assertEquals('%D5%95', encodeURI(s4));
assertEquals('%DF%BF', encodeURI(s5));
assertEquals('%E0%A0%80', encodeURI(s6));
assertEquals('%EA%BB%AE', encodeURI(s7));
assertEquals('%F0%90%80%80', encodeURI(s8));
assertEquals('%F4%8F%BF%BF', encodeURI(s9));
assertEquals('%EE%80%80', encodeURI(s10));

assertEquals(cc1, decodeURI(encodeURI(s1)).charCodeAt(0));
assertEquals(cc2, decodeURI(encodeURI(s2)).charCodeAt(0));
assertEquals(cc3, decodeURI(encodeURI(s3)).charCodeAt(0));
assertEquals(cc4, decodeURI(encodeURI(s4)).charCodeAt(0));
assertEquals(cc5, decodeURI(encodeURI(s5)).charCodeAt(0));
assertEquals(cc6, decodeURI(encodeURI(s6)).charCodeAt(0));
assertEquals(cc7, decodeURI(encodeURI(s7)).charCodeAt(0));
assertEquals(cc8_1, decodeURI(encodeURI(s8)).charCodeAt(0));
assertEquals(cc8_2, decodeURI(encodeURI(s8)).charCodeAt(1));
assertEquals(cc9_1, decodeURI(encodeURI(s9)).charCodeAt(0));
assertEquals(cc9_2, decodeURI(encodeURI(s9)).charCodeAt(1));
assertEquals(cc10, decodeURI(encodeURI(s10)).charCodeAt(0));
