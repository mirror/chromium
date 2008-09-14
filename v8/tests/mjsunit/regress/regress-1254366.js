function gee() {};

Object.prototype.findOrStore = function() {
  var z = this.vvv = gee;
  return z;
};

var a =  new Object();
assertEquals(gee, a.findOrStore());
assertEquals(gee, a.findOrStore());

