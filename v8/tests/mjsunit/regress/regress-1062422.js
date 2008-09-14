// 1062422 Ensure that accessors can handle unexpected receivers.
Number.prototype.__proto__ = String.prototype;
assertEquals((123).length, 0)
