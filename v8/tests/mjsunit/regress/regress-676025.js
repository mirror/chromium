var result;
try { eval('a=/(/'); } catch (e) { result = e; }
assertEquals('object', typeof result);
assertTrue(result instanceof SyntaxError);
