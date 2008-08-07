// Properties in prototypes should be accessible from within 'with'
// statements.

var errors = 0;
function assert(p) {
  if (!p) {
    print("ASSERTION FAILED");
    errors++;
  }
}


(function() {
  var x = {};
  with (x) { assert(typeof toString == 'function'); }
})();


if (errors == 0) print("PASSED");
