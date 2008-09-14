// Test of repeated unshift, shift and splice operations.
// The first benchmark uses call, the second calls the operations directly.

var a = [];


function assertLength(array, length) {
  if (array.length != length) {
    throw new Error("Wasn't expecting array to have length " + array.length);
  }
}


function test(fMethod, bAdd, iIterations) {
  while (bAdd ? a.length < iIterations : a.length > 0) {
    fMethod.call(a, 0, 1);
  }
}


function benchmark() {
  a = [];
  var iIterations = 1000;
  assertLength(a, 0);
  test(Array.prototype.push, true,  iIterations);
  assertLength(a, iIterations);
  test(Array.prototype.pop,   false, iIterations);
  assertLength(a, 0);
  test(Array.prototype.unshift, true,  iIterations);
  assertLength(a, iIterations);
  test(Array.prototype.shift,   false, iIterations);
  assertLength(a, 0);
  while (a.length < iIterations) a.push(0); // Fill array for next tests
  assertLength(a, iIterations);
  test(Array.prototype.splice,  false, iIterations);
  assertLength(a, 0);
}


function test2push(iIterations) {
  while (a.length < iIterations) {
    a.push(0, 1);
  }
}


function test2pop(iIterations) {
  while (a.length > 0) {
    a.pop();
  }
}


function test2unshift(iIterations) {
  while (a.length < iIterations) {
    a.unshift(0, 1);
  }
}


function test2shift(iIterations) {
  while (a.length > 0) {
    a.shift();
  }
}


function test2splice(iIterations) {
  while (a.length > 0) {
    a.splice(0, 1);
  }
}



function benchmark2() {
  a = [];
  var iIterations = 1000;
  assertLength(a, 0);
  test2push(iIterations);
  assertLength(a, iIterations);
  test2pop(iIterations);
  assertLength(a, 0);
  test2unshift(iIterations);
  assertLength(a, iIterations);
  test2shift(iIterations);
  assertLength(a, 0);
  while (a.length < iIterations) a.push(0); // Fill array for next tests
  assertLength(a, iIterations);
  test2splice(iIterations);
  assertLength(a, 0);
}


var elapsed = 0;
var start = new Date();
for (var n = 0; elapsed < 2000; n++) {
  benchmark();
  elapsed = new Date() - start;
}
print('Time (array-ops-call): ' + Math.floor(elapsed/n) + ' ms.');

elapsed = 0;
start = new Date();
for (var n = 0; elapsed < 2000; n++) {
  benchmark2();
  elapsed = new Date() - start;
}
print('Time (array-ops): ' + Math.floor(elapsed/n) + ' ms.');
