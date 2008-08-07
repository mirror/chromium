function benchmark() {
  try {
    load("lh_experimental_en_US.js");
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
  print('Time (parse-lh-experimental-en-US): ' + Math.floor(1000 * elapsed/n) + ' us.');
}

run();
