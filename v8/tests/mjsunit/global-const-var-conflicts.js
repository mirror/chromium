// Check that dynamically introducing conflicting consts/vars
// leads to exceptions.

var caught = 0;

eval("const a");
try { eval("var a"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertTrue(typeof a == 'undefined');
try { eval("var a = 1"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertTrue(typeof a == 'undefined');

eval("const b = 0");
try { eval("var b"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertEquals(0, b);
try { eval("var b = 1"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertEquals(0, b);

eval("var c");
try { eval("const c"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertTrue(typeof c == 'undefined');
try { eval("const c = 1"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertTrue(typeof c == 'undefined');

eval("var d = 0");
try { eval("const d"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertEquals(0, d);
try { eval("const d = 1"); } catch (e) { caught++; assertTrue(e instanceof TypeError); }
assertEquals(0, d);

assertEquals(8, caught);
