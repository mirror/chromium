/* Many of the Mozilla regexp tests used 'toSource' to test their
 * results.  Since we don't currently support toSource, those tests
 * are disabled and standalone versions are included here.
 */

// Tests from ecma_3/RegExp/regress-78156.js
var string = 'aaa\n789\r\nccc\r\n345';
var pattern = /^\d/gm;
var result = string.match(pattern);
assertEquals(2, result.length, "1");
assertEquals('7', result[0], "2");
assertEquals('3', result[1], "3");

pattern = /\d$/gm;
result = string.match(pattern);
assertEquals(2, result.length, "4");
assertEquals('9', result[0], "5");
assertEquals('5', result[1], "6");

string = 'aaa\n789\r\nccc\r\nddd';
pattern = /^\d/gm;
result = string.match(pattern);
assertEquals(1, result.length, "7");
assertEquals('7', result[0], "8");

pattern = /\d$/gm;
result = string.match(pattern);
assertEquals(1, result.length, "9");
assertEquals('9', result[0], "10");

// Tests from ecma_3/RegExp/regress-72964.js
pattern = /[\S]+/;
string = '\u00BF\u00CD\u00BB\u00A7';
result = string.match(pattern);
assertEquals(1, result.length, "11");
assertEquals(string, result[0], "12");

string = '\u00BF\u00CD \u00BB\u00A7';
result = string.match(pattern);
assertEquals(1, result.length, "13");
assertEquals('\u00BF\u00CD', result[0], "14");

string = '\u4e00\uac00\u4e03\u4e00';
result = string.match(pattern);
assertEquals(1, result.length, "15");
assertEquals(string, result[0], "16");

string = '\u4e00\uac00 \u4e03\u4e00';
result = string.match(pattern);
assertEquals(1, result.length, "17");
assertEquals('\u4e00\uac00', result[0], "18");
