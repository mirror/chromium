function f(s) {
  print(eval(s));
}

// Expression statement
f('');
f('1');
f('2;');
f('3;4');
f('5;6;');

// Block statement
f('{}');
f('{7}');
f('{8};');
f('{9;}');
f('{10;11;}');

// If statement
f('if (true) 12');
f('if (false) 13');
f('if (true) 14; else 15;');
f('if (true) 16; else 17;');
f('18; if (true) 19;');
f('20; if (false) 21;');

// If statement regressions; see rewriter.cc:VisitIfStatement(...)

// -- with else
f('0; if (true) ; else var x;'); // SAI = false; SAE = false; SAT = false
f('0; if (false) ; else var x;'); // SAI = false; SAE = false; SAT = false
f('0; if (true) 1; else var x;'); // SAI = false; SAE = false; SAT = true
f('0; if (false) 1; else var x;'); // SAI = false; SAE = false; SAT = true

f('0; if (true) ; else 1;'); // SAI = false; SAE = true; SAT = false
f('0; if (false) ; else 1;'); // SAI = false; SAE = true; SAT = false
f('0; if (true) 1; else 2;'); // SAI = false; SAE = true; SAT = true
f('0; if (false) 1; else 2;'); // SAI = false; SAE = true; SAT = true

// TODO(kasperl): There is more stuff to check here.

// -- without else
f('0; if (true);'); // SAE = false; SAT = false
f('0; if (true) 1'); // SAE = false; SAT = true
f('L: { 0; if (true) break L; 1 }'); // SAE = true; SAT = false
f('0; if (true); 2'); // SAE = true; SAT = true


// Variable statement
f('var x;');
f('var x = 22;');
f('23; var x;');
f('24; var x = 25;');

// Do-while statement.
f('do { } while (false)');
f('26; do { } while (false)');
f('do { 27 } while (false)');
f('28; do { 29 } while (false)');

// While statement.
f('while (false) { 30; }');
f('31; while (false) { 32; }');
f('var c = true; while (c) { c = false; 33 }');
f('34; var c = true; while (c) { c = false }');

// For statement.
f('for (35; false; 36) { 37 }');
f('38; for (39; false; 40) { 41 }');
f('var c = true; for (42; c; 43) { c = false; 44 }');
f('45; var c = true; for (46; c; 47) { c = false }');
f('var c = true; for (48; c; c = false) { 49 }');
f('50; var c = true; for (51; c; c = false) { 52 }');

// For-in statement.
f('for (var i in {}) { 53 }');
f('for (var i in [54]) { 55 }'); 

// Break statement.
f('56; L: { break L; 57; }');
f('58; L: { break L; 59; } 60');
f('61; L: { 62; break L; 63; }');
f('64; L: { 65; break L; 66; } 67');

// Continue statement.
f('var c = true; while (c) { c = false; continue; 68; }');
f('69; var c = true; while (c) { c = false; 70; continue; 71; }');
f('72; var c = true; while (c) { c = false; 73; if (!c) continue; 74; }');
f('75; var c = true; while (c) { c = false; 76; if (!c) { 77; continue } }');

// With statement.
// NOT IMPLEMENTED YET!

// Switch statement.
f('switch (0) { case 0: 78; break; }');
f('switch (0) { case 0: 79; break; case 1: 80}');
f('switch (0) { case 0: 81; case 1: 82;}');
f('switch (1) { case 0: 83; break; default: 84; break; }');

// Labelled statement.
f('L: 85');
f('L: { 86 }');

// Try-catch statement.
f('try { } catch (o) { }');
f('try { 87 } catch (o) { }');
f('try { 88 } catch (o) { 89 }');
f('try { 90; throw 91; } catch (o) { }');
f('try { 92; throw 93; } catch (o) { 94 }');
f('try { throw 95; } catch (o) { }');

// Try-finally statement.
f('try { } finally { }');
f('try { 96 } finally { }');
f('try { 97 } finally { 98 }');

// Function declaration statement.
f('function() {}');
f('function g() {}');




