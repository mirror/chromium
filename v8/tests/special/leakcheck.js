/**
 * This test is run with leak detection when running special tests.
 * Don't do too much work here or running it will take forever.
 */

function fac(n) {
  if (n > 0) return fac(n - 1) * n;
  else return 1;
}

function testFac() {
  if (fac(6) != 720) throw "Error";
}

function testRegExp() {
  var input = "123456789";
  var result = input.replace(/[4-6]+/g, "xxx");
  if (result != "123xxx789") throw "Error";
}

function main() {
  testFac();
  testRegExp();
}

main();
