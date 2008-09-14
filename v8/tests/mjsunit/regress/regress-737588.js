var goog = goog || {} ;
goog.global = this;
goog.globalEval = function(script) {
  return goog.global.eval(script);
};

assertEquals(125, goog.globalEval('var foofoofoo = 125; foofoofoo'));
