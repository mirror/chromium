const TESTS = [
    "bintree.js",
    "richards.js",
    "delta-blue.js",
    "fannkuch.js",
    "cryptobench.js",
    "evalbench.js",
    "lisp.js",
    "fib.js"
];


var seed = SEED;
function RandomTest() {
  var n = 36969 * seed;
  seed = (n + (n & 0xffff)) & 0x3fffffff;
  var index = seed % TESTS.length;
  return TESTS[index];
}


function RunTests() {
  var milliseconds = SECONDS * 1000;
  var start = Date.now();
  while (Date.now() - start < milliseconds) {
    var test = RandomTest();
    print('Running ' + test);
    load(PREFIX + test);
  }
}


RunTests();
