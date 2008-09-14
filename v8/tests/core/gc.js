/* Sample: Test GC. */

function f() {
  gc();
}

function gp0(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return p0;
}

function gp1(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return p1;
}

function gp2(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return p2;
}

print(gp0('Okay', 'Fail', 'Fail'));
print(gp1('Fail', 'Okay', 'Fail'));
print(gp2('Fail', 'Fail', 'Okay'));


function gl0(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return l0;
}

function gl1(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return l1;
}

function gl2(p0, p1, p2) {
  var l0 = p0;
  var l1 = p1;
  var l2 = p2;
  f();
  return l2;
}

print(gl0('Okay', 'Fail', 'Fail'));
print(gl1('Fail', 'Okay', 'Fail'));
print(gl2('Fail', 'Fail', 'Okay'));


