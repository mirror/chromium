var new_inner_functions_semantics = false;

var actual = '';
var expect = (new_inner_functions_semantics ? 'global' : 'with');

//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------

function test() {
  print("Entering endless loop?")
  var x = "global"

  with ({x: "with"})
    actual = (function() { return x }());
    // actual = x;
  
  print(expect == actual);
}
