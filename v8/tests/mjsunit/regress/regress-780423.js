var Class = {
 create: function() {
   return function kurt() {
   }
 }
};

var o1 = Class.create();
var o2 = Class.create();

assertTrue(o1 !== o2, "different functions");
assertTrue(o1.prototype !== o2.prototype, "different protos");
