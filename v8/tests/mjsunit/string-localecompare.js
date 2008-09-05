// String.prototype.localeCompare.
//
// Implementation dependent function.  For now, we do not do anything
// locale specific.

// Equal prefix.
assertTrue("asdf".localeCompare("asdf") == 0);
assertTrue("asd".localeCompare("asdf") < 0);
assertTrue("asdff".localeCompare("asdf") > 0);

// Chars differ.
assertTrue("asdf".localeCompare("asdq") < 0);
assertTrue("asdq".localeCompare("asdf") > 0);
