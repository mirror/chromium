assertEquals("object", typeof this);
var threw = false;
try {
  this();
} catch (e) {
  threw = true;
}
assertTrue(threw);
