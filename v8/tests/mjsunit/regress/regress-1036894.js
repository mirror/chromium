xeval = function(s) { eval(s); }
xeval("$=function anonymous() { /*noex*/do {} while(({ get x(x) { break ; }, set x() { (undefined);} })); }");

foo = function() { eval("$=function anonymous() { /*noex*/do {} while(({ get x(x) { break ; }, set x() { (undefined);} })); }"); }
foo();

xeval = function(s) { eval(s); }
eval("$=function anonymous() { /*noex*/do {} while(({ get x(x) { break ; }, set x() { (undefined);} })); }");

xeval = function(s) { eval(s); }
xeval('$=function(){L: {break L;break L;}};');
