// Bug ecma/String/15.5.4.js
//
// After assignment to 'String.prototype.getClass' and 'delete' (see code below),
// all String properties become suddenly enumarable again.

function GetStringProperties() {
	var value = '';
	for (var prop in "abc") {
	  value += prop + ",";
	}
	return value;
}

print("1: String properties (should be '0,1,2,'): '" + GetStringProperties() + "'");

String.prototype.getClass = Object.prototype.toString;
print("2: String properties (should be '0,1,2,getClass,'): '" + GetStringProperties() + "'");

delete String.prototype.getClass;
print("3: String properties (should be '0,1,2,'): '" + GetStringProperties() + "'");

