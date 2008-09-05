print(typeof 4);
print(typeof 3.1415926);
print(typeof null);
print(typeof undefined);
print(typeof (new Object));
print(typeof true);
print(typeof false);
print(typeof "David Hasselhoff");
print(typeof "");
print(typeof function () { });
function f() { }
print(typeof f);
print(typeof {}.missingProperty);

function type(x) {
  if (typeof x === 'aksdlfjl') {
    return 100;
  } else if (typeof x === 'number') {
    return 1;
  } else if (typeof x === 'string') {
    return 2;
  } else if (typeof x === 'boolean') {
    return 3;
  } else if (typeof x === 'function') {
    return 4;
  } else if (typeof x === 'object') {
    return 5;
  } else if (typeof x === 'undefined') {
    return 6;
  } else {
    return 200;
  }
}

var a = [ 1, new Number(1), Number.NaN,
          'foo', new String('foo'), 
          true, false, new Boolean(1), new Boolean(false), 
          function () { }, type,
          undefined ]

for (var i = 0; i < a.length; i++) print(type(a[i]));
