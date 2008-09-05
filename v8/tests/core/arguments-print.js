(function (x, y) {
  function g(a) {
    a[0] = "three";
    return a.length;
  }

  var l = g(arguments);
  y = 5;
  print(l, x, y);
})(3, "five");  // 2 three 5


print(function () {
    if (arguments[0] > 0) return arguments.callee(arguments[0] - 1) + arguments[0];
    return 0;
  } (10));  // 55


(function () {
  print(arguments.length);
})();  // 0


(function () {
  var arguments = 0;
  print(arguments.length);
})();  // undefined


(function (x, y, z) {
  function g(a) {
    x = "two";
    y = "three";
    a[1] = "drei";
    a[2] = "fuenf";
  };

  g(arguments);
  print(x, y, z);
})(2, 3, 5);  // two drei fuenf
