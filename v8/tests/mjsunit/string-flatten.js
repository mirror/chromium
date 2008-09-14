// Check for regression of bug 1011505 (out of memory when flattening strings).
var i;
var s = "";

for (i = 0; i < 1024; i++) {
  s = s + (i + (i+1));
  s = s.substring(1);
}
// Flatten the string.
assertEquals(57, s.charCodeAt(0));
