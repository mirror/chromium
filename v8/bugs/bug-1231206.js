function TypeOfThis() { return typeof this; }

String.prototype.TypeOfThis = TypeOfThis;
Boolean.prototype.TypeOfThis = TypeOfThis;
Number.prototype.TypeOfThis = TypeOfThis;

assertEquals('object', 'xxx'.TypeOfThis());
assertEquals('object', true.TypeOfThis());
assertEquals('object', (42).TypeOfThis());
