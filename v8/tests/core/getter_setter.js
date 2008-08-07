function check_getter_setter(obj) {
  var sum = 0;
  for(var name in obj) sum += obj[name];

  print(obj.fisk);
  print(obj["kalle"]);
  print(sum);
  obj.kalle = 4;
  
  print(typeof obj.__lookupGetter__("kalle") == 'function');
  print(typeof obj.__lookupSetter__("kalle") == 'function');
}

// Check setter and getter defined by primitives.
var a = { fisk : 2 }
a.__defineGetter__("kalle", function() { return this.fisk * 2; });
a.__defineSetter__("kalle", function(value) { print(this.fisk * value); });
check_getter_setter(a);

// Check setter and getter defined by object literal.
check_getter_setter({
  get kalle() { return this.fisk * 2; },
  set kalle(value) { print(this.fisk * value); },
  fisk: 2
})

// Verify getter can overwrite a normal property.
var o = { x: 'old x', y: 'old y' };
o.__defineGetter__('x', function() { return 'new x'; });
print(o.x);

// Verify setter can overwrite a normal property.
o.__defineSetter__('y', function(value) { this.y_mirror = value; });
o.y = 'new y';
print(o.y_mirror);
