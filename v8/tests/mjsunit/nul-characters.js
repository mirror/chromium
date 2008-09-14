var a = [ '\0', '\u0000', '\x00' ]
for (var i in a) {
  assertEquals(1, a[i].length);
  assertEquals(0, a[i].charCodeAt(0));
}

assertEquals(7, 'foo\0bar'.length);
assertEquals(7, 'foo\x00bar'.length);
assertEquals(7, 'foo\u0000bar'.length);

assertEquals(2, ('\0' + '\0').length);
