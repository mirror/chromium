JsUnitException.prototype.toString = function () {
  return this.jsUnitMessage + ' ' + this.stackTrace;
};


for (var candidate in this) {
  if ('string' === (typeof candidate) && /^test/.test(candidate) &&
    'function' === (typeof this[candidate])) {
    print("running " + candidate);
    try {
      this[candidate]();
    } catch (ex) {
      print(" - exception: " + ex);
    }
  }
}
