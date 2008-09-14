// The [[Class]] property of (instances of) builtin functions must be
// correctly set.
var funs = {
  Object:   [ Object ],
  Function: [ Function ],
  Array:    [ Array ],
  String:   [ String ],
  Boolean:  [ Boolean ],
  Number:   [ Number ],
  Date:     [ Date ],
  RegExp:   [ RegExp ],  
  Error:    [ Error, TypeError, RangeError, SyntaxError, ReferenceError, EvalError, URIError ]
}
for (f in funs) {
  for (i in funs[f]) {
    assertEquals("[object " + f + "]",
                 Object.prototype.toString.call(new funs[f][i]),
                 funs[f][i]);
    assertEquals("[object Function]",
                 Object.prototype.toString.call(funs[f][i]),
                 funs[f][i]);
  }
}
