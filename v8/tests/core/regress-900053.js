// This is not necessary a bug in V8. Google's closure code assumes that
// for-in enumerates properties in the order of insertion. Firefox, Safari,
// Opera and IE all follow this on local properties, and Rhino does not.
// If a local property is deleted and re-inserted, Firefox, Safari, and Opera
// put the property at the end, but IE puts the property in its original 
// place.
// 
// When an object has prototypes, FF, Safari, Opera enumerate local properties
// then prototype properties in insertion order.
// IE enumerates prototype properties first.
//

function object_to_string(obj) {
  var r = '';
  for (var p in obj) {
    r += obj[p];
  }
  return r;
}

// add properties to prototype
function myclass() {}
myclass.prototype = new Object;
var n = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';

// add properties using reversed order
for (var i = n.length - 1; i >= 0; i--) {
  myclass.prototype[n.charAt(i)] = n.charAt(i);
}

// add properties to normal type
var s = 'abcdefghijklmnopqrstuvwxyz';
var o = new myclass;
// add properties using reversed order
for (var i = s.length - 1; i >= 0; i--) {
  o[s.charAt(i)] = s.charAt(i);
}

// should print out both normal properties and properties in the prototype
print(object_to_string(o));

// replace a value
o.g = 'g';
print(object_to_string(o));

// delete a property
delete o.x;
print(object_to_string(o));

// reinsert the property
o.x = 'x';
print(object_to_string(o));

// delete a property in the prototype
delete myclass.prototype['G'];
print(object_to_string(o));

// shadow a property in the prototype
o['A'] = 'A';
print(object_to_string(o));
