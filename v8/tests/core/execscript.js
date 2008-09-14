// Test execScript.
//
// This code mimics code used in google closure library for doing
// eval in the global context.

function check(x, x0) {
  if (x != x0)
    print("got " + x + "; expected " + x0);
}

var goog = goog || {};

goog.global = this;

goog.globalEval = function(script) {
  if (goog.global.execScript) {
    goog.global.execScript(script, 'JavaScript');
  } else if (goog.global.eval) {
    goog.global.eval(script);
  } else {
    throw Error('goog.globalEval not available');
  }
};

function testGlobalEval() {
  goog.globalEval('var foo = 125;');
  check(goog.global.foo, 125);
  var foo = 128;
  check(goog.global.foo, 125);
}

testGlobalEval();
