(function f(x) {
  function g(x) {
    print(x);
    f(x-1);
  }
  if (x != 0) g(x);  
})(7);
