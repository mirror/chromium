// Implementation of factorial(n). Integers are represented as
// lists of "successors of zero", and the lists themselves are
// represented as closures (JS functions). Evaluating such a
// function will yield the predecessor closure; the value zero
// is represented by a closure which has itself as the predecessor.
// (gri 2/25/2007).

// -------------------------------------
// Peano primitives

function zero() {
  return zero;
}


function is_zero(x) {
  return x == x();
}


function add1(x) {
  return function() { return x; };
}


function add(x, y) {
  if (is_zero(y))
    return x;

  return add(add1(x), y());
}


function mul(x, y) {
  if (is_zero(y))
    return zero();

  if (is_zero(y()))
    return x;

  return add(mul(x, y()), x);
}


function fact(n) {
  if (is_zero(n))
    return add1(zero());

  return mul(fact(n()), n);
}


// -------------------------------------
// Helpers to generate/count Peano integers

function gen(n) {
  if (n > 0)
    return add1(gen(n - 1));

  return zero();
}


function count(x) {
  if (is_zero(x))
    return 0;

  return count(x()) + 1;
}


function check(x, expected) {
  var c = count(x);
  if (c != expected) {
    print("error: found " + c + "; expected " + expected);
  }
}


// -------------------------------------
// Test basic functionality

check(zero(), 0);
check(add1(zero()), 1);
check(gen(10), 10);

check(add(gen(3), zero()), 3);
check(add(zero(), gen(4)), 4);
check(add(gen(3), gen(4)), 7);

check(mul(zero(), zero()), 0);
check(mul(gen(3), zero()), 0);
check(mul(zero(), gen(4)), 0);
check(mul(gen(3), add1(zero())), 3);
check(mul(add1(zero()), gen(4)), 4);
check(mul(gen(3), gen(4)), 12);

check(fact(zero()), 1);
check(fact(add1(zero())), 1);
check(fact(gen(5)), 120);


// -------------------------------------
// Timing

function time_it(f) {
  var t0 = new Date();
  f();
  var t1 = new Date();
  print("time = " + (t1.getTime() - t0.getTime()));
}


// -------------------------------------
// Factorial
//
// v8 can't go higher then 8! because of stack overflow;
// but most other JS implementations won't go beyond 7!.
// Unfortunately, v8 only goes to 7 on windows. (ringgaard)

for (i = 0; i <= 7; i++)
  print(i + "! = " + count(fact(gen(i))));
