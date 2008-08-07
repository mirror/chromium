// Test the exponential notation output.
assertEquals("1e+27", (1.2345e+27).toPrecision(1));
assertEquals("1.2e+27", (1.2345e+27).toPrecision(2));
assertEquals("1.23e+27", (1.2345e+27).toPrecision(3));
assertEquals("1.234e+27", (1.2345e+27).toPrecision(4));
assertEquals("1.2345e+27", (1.2345e+27).toPrecision(5));
assertEquals("1.23450e+27", (1.2345e+27).toPrecision(6));
assertEquals("1.234500e+27", (1.2345e+27).toPrecision(7));

assertEquals("-1e+27", (-1.2345e+27).toPrecision(1));
assertEquals("-1.2e+27", (-1.2345e+27).toPrecision(2));
assertEquals("-1.23e+27", (-1.2345e+27).toPrecision(3));
assertEquals("-1.234e+27", (-1.2345e+27).toPrecision(4));
assertEquals("-1.2345e+27", (-1.2345e+27).toPrecision(5));
assertEquals("-1.23450e+27", (-1.2345e+27).toPrecision(6));
assertEquals("-1.234500e+27", (-1.2345e+27).toPrecision(7));


// Test the fixed notation output.
assertEquals("7", (7).toPrecision(1));
assertEquals("7.0", (7).toPrecision(2));
assertEquals("7.00", (7).toPrecision(3));

assertEquals("-7", (-7).toPrecision(1));
assertEquals("-7.0", (-7).toPrecision(2));
assertEquals("-7.00", (-7).toPrecision(3));

assertEquals("9e+1", (91).toPrecision(1));
assertEquals("91", (91).toPrecision(2));
assertEquals("91.0", (91).toPrecision(3));
assertEquals("91.00", (91).toPrecision(4));

assertEquals("-9e+1", (-91).toPrecision(1));
assertEquals("-91", (-91).toPrecision(2));
assertEquals("-91.0", (-91).toPrecision(3));
assertEquals("-91.00", (-91).toPrecision(4));

assertEquals("9e+1", (91.1234).toPrecision(1));
assertEquals("91", (91.1234).toPrecision(2));
assertEquals("91.1", (91.1234).toPrecision(3));
assertEquals("91.12", (91.1234).toPrecision(4));
assertEquals("91.123", (91.1234).toPrecision(5));
assertEquals("91.1234", (91.1234).toPrecision(6));
assertEquals("91.12340", (91.1234).toPrecision(7));
assertEquals("91.123400", (91.1234).toPrecision(8));

assertEquals("-9e+1", (-91.1234).toPrecision(1));
assertEquals("-91", (-91.1234).toPrecision(2));
assertEquals("-91.1", (-91.1234).toPrecision(3));
assertEquals("-91.12", (-91.1234).toPrecision(4));
assertEquals("-91.123", (-91.1234).toPrecision(5));
assertEquals("-91.1234", (-91.1234).toPrecision(6));
assertEquals("-91.12340", (-91.1234).toPrecision(7));
assertEquals("-91.123400", (-91.1234).toPrecision(8));

