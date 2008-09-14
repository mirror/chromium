function F() {
  return arguments.length;
}

assertEquals(0, F.apply(), "no receiver or args");
assertEquals(0, F.apply(this), "no args");
assertEquals(0, F.apply(this, []), "empty args");
assertEquals(0, F.apply(this, [], 0), "empty args, extra argument");
