// global functions/properties are not yet treated like local properties, which
// causes problems due to assumptions by the IC mechanism.
function f() {};
f = 0;
