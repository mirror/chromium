// Regression test for bug #743664.
assertEquals("\x60\x60".replace(/\x60/g, "u"), "uu");
assertEquals("\xAB\xAB".replace(/\xAB/g, "u"), "uu");
