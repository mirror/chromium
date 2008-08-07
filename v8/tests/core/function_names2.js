// This is a somewhat contrived piece of code that caused problems for V8
// because the allocation order of the function variable corresponding to
// the function literal inside g was not allocated in the right order with
// respect to the variable e in the inner catch scope. It caused an assertion
// failure at compile time.

function f(a) {
  (function g() {
    a;
    try {
    } catch (e) {
    }
  })
}

f(0);
print("PASSED");
