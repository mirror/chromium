// Completely artificial String.prototype.indexOf benchmark.

function benchmark() {
  for (var i = 0; i < 10000; i++) {
    for (var j = 0; j < 25; j++) {
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx".indexOf("y", j);
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx".indexOf("yyy", j);
      "xxxxyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxyxx".indexOf("y", j);
      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxyyyxxxxxxxx".indexOf("yyy", j);
    }
  }
}

var elapsed = 0;
var start = new Date();
for (var n = 0; elapsed < 2000; n++) {
  benchmark();
  elapsed = new Date() - start;
}
print('Time (string-indexof): ' + Math.floor(elapsed/n) + ' ms.');
