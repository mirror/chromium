expected = ["A", undefined, "B", "bold", "/", "B", "and", undefined, "CODE", "coded", "/", "CODE", ""];
result = "A<B>bold</B>and<CODE>coded</CODE>".split(/<(\/)?([^<>]+)>/);
assertArrayEquals(expected, result, 1);

expected = ["a", "b"];
result = "ab".split(/a*?/);
assertArrayEquals(expected, result, 2);

expected = ["", "b"];
result = "ab".split(/a*/);
assertArrayEquals(expected, result, 3);

expected = ["a"];
result = "ab".split(/a*?/, 1);
assertArrayEquals(expected, result, 4);

expected = [""];
result = "ab".split(/a*/, 1);
assertArrayEquals(expected, result, 5);

expected = ["as","fas","fas","f"];
result = "asdfasdfasdf".split("d");
assertArrayEquals(expected, result, 6);

expected = ["as","fas","fas","f"];
result = "asdfasdfasdf".split("d", -1);
assertArrayEquals(expected, result, 7);

expected = ["as", "fas"];
result = "asdfasdfasdf".split("d", 2);
assertArrayEquals(expected, result, 8);

expected = [];
result = "asdfasdfasdf".split("d", 0);
assertArrayEquals(expected, result, 9);

expected = ["as","fas","fas",""];
result = "asdfasdfasd".split("d");
assertArrayEquals(expected, result, 10);

expected = [];
result = "".split("");
assertArrayEquals(expected, result, 11);

expected = [""]
result = "".split("a");
assertArrayEquals(expected, result, 12);

expected = ["a","b"]
result = "axxb".split(/x*/);
assertArrayEquals(expected, result, 13);

expected = ["a","b"]
result = "axxb".split(/x+/);
assertArrayEquals(expected, result, 14);

expected = ["a","","b"]
result = "axxb".split(/x/);
assertArrayEquals(expected, result, 15);

// This was http://b/issue?id=1151354
expected = ["div", "#id", ".class"]
result = "div#id.class".split(/(?=[#.])/);
assertArrayEquals(expected, result, 16);

expected = ["div", "#i", "d", ".class"]
result = "div#id.class".split(/(?=[d#.])/);
assertArrayEquals(expected, result, 17);

expected = ["a", "b", "c"]
result = "abc".split(/(?=.)/);
assertArrayEquals(expected, result, 18);

/* "ab".split(/((?=.))/)
 * 
 * KJS:   ,a,,b
 * SM:    a,,b,
 * IE:    a,b
 * Opera: a,,b
 * V8:    a,,b
 * 
 * Opera seems to have this right.  The others make no sense.
 */
expected = ["a", "", "b"]
result = "ab".split(/((?=.))/);
assertArrayEquals(expected, result, 19);

/* "ab".split(/(?=)/)
 *
 * KJS:   a,b
 * SM:    ab
 * IE:    a,b
 * Opera: a,b
 * V8:    a,b
 */
expected = ["a", "b"]
result = "ab".split(/(?=)/);
assertArrayEquals(expected, result, 20);

