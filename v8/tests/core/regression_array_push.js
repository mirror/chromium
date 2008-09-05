// This tests makes sure we do not execute the optimized version on array.prototype.{pop, push} on
// unoptimized arrays.

var array = new Array();
array[23] = 0;
array.push(1);
array.pop();
