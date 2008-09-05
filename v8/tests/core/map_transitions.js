// Regression test for ensuring that b does not end up with the same map as a.
var a;
var b;
function pA(){ print('hest'); }
function bF(){}
a=new bF();a.tN='JavaScriptObject';
b=new bF();b.g=pA;b.tN='UIObject';
b.g();
