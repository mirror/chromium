try { try { print('nested'); } catch (o) { } } catch (o) { } 

try {
  print('hello');
  throw { x: 12 };
  print('world');
} catch (o) {
  print(o.x);
}

function foo(x) {
  gc();
  throw x;
}

try {
  print('nice'); 
  foo(87);
  print('try');
} catch (o) {
  print(o);
}


function bar() {
  gc();
  try {
    gc();
    return 1;
  } catch (o) {
    return 2;
  }
}

try {
  print(bar());
  throw 'okay';
  print('never here');
} catch (o) {
  print(o);
}
