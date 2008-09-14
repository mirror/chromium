
function check_enumeration_order(obj)  {
  var value = 0; 
  for (var name in obj) assertTrue(value < obj[name]);
  value = obj[name];
}

function make_object(size)  {
  var a = new Object();
  
  for (var i = 0; i < size; i++) a["a_" + i] = i + 1;
  check_enumeration_order(a);
  
  for (var i = 0; i < size; i +=3) delete a["a_" + i];
  check_enumeration_order(a);
}

// Validate the enumeration order for object up to 100 named properties.
for (var j = 1; j< 100; j++) make_object(j);


function make_literal_object(size)  {
  var code = "{ ";
  for (var i = 0; i < size-1; i++) code += " a_" + i + " : " + (i + 1) + ", ";
  code += "a_" + (size - 1) + " : " + size;
  code += " }";
  eval("var a = " + code);
  check_enumeration_order(a);  
}

// Validate the enumeration order for object literals up to 100 named properties.
for (var j = 1; j< 100; j++) make_literal_object(j);

