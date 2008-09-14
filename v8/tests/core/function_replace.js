// Regression for bug 782216

var obj = { };

for (var i = 0; i < 1000; i++) {
  obj['p' + i] = function () { };
  obj['p' + i] = new Object();
}

