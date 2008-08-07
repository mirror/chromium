var obj = new (Function.__proto__)();


var threw = false;
try {
  obj.toString();
} catch (e) {
  assertInstanceof(e, TypeError);
  threw = true;
}
assertTrue(threw);
