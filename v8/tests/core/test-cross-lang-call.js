// test simple parameter passing
function callback_add(a, b) {
  return a+b;
}

var c = eval("callback_add(1, 2)");
if (c != 3) {
  print("failure, expecting 3, got "+c);
}

// test parameter passing and gc
function callback_gc_add(a, b) {
  gc();
  return a+b;
}

var c = eval("callback_gc_add(1, 2)");
if (c != 3) {
  print("failure, expecting 3, got "+c);
}

// test throw exception
function callback_throw(a) {
  throw a;
}

try {
  eval("callback_throw(10)");
} catch (o) {
  if (o != 10)
    print("failure, expecting 10, got "+c);
}

// test throw exception and gc
function callback_gc_throw(a) {
  gc();
  throw a;
}

try {
  eval("callback_gc_throw(10)");
} catch (o) {
  if (o != 10)
    print("failure, expecting 10, got "+c);
}

// test receiver
var c = {a : 5}
c.callback_this_add = function (b) {
  this.a += b;
}

eval("c.callback_this_add(3)");
if (c.a != 8) {
  print("failure, expecting 8, got "+c.a);
}

// test nested try/catch
function callback_try_catch() {
  try {
    throw 12;
  } catch (o) {
    if (o != 12) 
      print("failure, expecting 12, got "+o);
  }
}

try {
  eval("callback_try_catch()");  // shound not throw exception 
  eval("callback_throw(10)");
} catch (o) {
  if (o != 10)
    print("failure, expecting 10, got "+o);
}

print("done");
