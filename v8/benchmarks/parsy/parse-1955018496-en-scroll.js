function benchmark() {
  try {
    load("1955018496-en-scroll.js");
  } catch (e) {
    // Do nothing.
  }
}


function run() {
  var elapsed = 0;
  benchmark();
  var start = new Date();
  for (var n = 0; elapsed < 2000; n++) {
    benchmark();
    elapsed = new Date() - start;
  }
  print('Time (parse-1955018496-en-scroll): ' + Math.floor(1000 * elapsed/n) + ' us.');
}

run();
