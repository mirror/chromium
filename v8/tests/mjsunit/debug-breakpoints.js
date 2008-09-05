// Flags: --expose-debug-as debug
// Get the Debug object exposed from the debug context global object.
Debug = debug.Debug

function f() {a=1;b=2};
function g() {
  a=1;
  b=2;
}

bp = Debug.setBreakPoint(f, 0, 0);
assertEquals("() {[B0]a=1;b=2}", Debug.showBreakPoints(f));
Debug.clearBreakPoint(bp);
assertEquals("() {a=1;b=2}", Debug.showBreakPoints(f));
bp1 = Debug.setBreakPoint(f, 0, 8);
assertEquals("() {a=1;[B0]b=2}", Debug.showBreakPoints(f));
bp2 = Debug.setBreakPoint(f, 0, 4);
assertEquals("() {[B0]a=1;[B1]b=2}", Debug.showBreakPoints(f));
bp3 = Debug.setBreakPoint(f, 0, 12);
assertEquals("() {[B0]a=1;[B1]b=2}[B2]", Debug.showBreakPoints(f));
Debug.clearBreakPoint(bp1);
assertEquals("() {[B0]a=1;b=2}[B1]", Debug.showBreakPoints(f));
Debug.clearBreakPoint(bp2);
assertEquals("() {a=1;b=2}[B0]", Debug.showBreakPoints(f));
Debug.clearBreakPoint(bp3);
assertEquals("() {a=1;b=2}", Debug.showBreakPoints(f));

// The following test checks that the Debug.showBreakPoints(g) produces output
// like follows when changein breakpoints.
//
// function g() {
//   [BX]a=1;
//   [BX]b=2;
// }[BX]

// Test set and clear breakpoint at the first possible location (line 0,
// position 0).
bp = Debug.setBreakPoint(g, 0, 0);
// function g() {
//   [B0]a=1;
//   b=2;
// }
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]a=1;") > 0);
Debug.clearBreakPoint(bp);
// function g() {
//   a=1;
//   b=2;
// }
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]") < 0);

// Second test set and clear breakpoints on lines 1, 2 and 3 (position = 0).
bp1 = Debug.setBreakPoint(g, 2, 0);
// function g() {
//   a=1;
//   [B0]b=2;
// }
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]b=2;") > 0);
bp2 = Debug.setBreakPoint(g, 1, 0);
// function g() {
//   [B0]a=1;
//   [B1]b=2;
// }
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]a=1;") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("[B1]b=2;") > 0);
bp3 = Debug.setBreakPoint(g, 3, 0);
// function g() {
//   [B0]a=1;
//   [B1]b=2;
// }[B2]
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]a=1;") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("[B1]b=2;") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("}[B2]") > 0);
Debug.clearBreakPoint(bp1);
// function g() {
//   [B0]a=1;
//   b=2;
// }[B1]
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]a=1;") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("}[B1]") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("[B2]") < 0);
Debug.clearBreakPoint(bp2);
// function g() {
//   a=1;
//   b=2;
// }[B0]
assertTrue(Debug.showBreakPoints(g).indexOf("}[B0]") > 0);
assertTrue(Debug.showBreakPoints(g).indexOf("[B1]") < 0);
Debug.clearBreakPoint(bp3);
// function g() {
//   a=1;
//   b=2;
// }
assertTrue(Debug.showBreakPoints(g).indexOf("[B0]") < 0);
