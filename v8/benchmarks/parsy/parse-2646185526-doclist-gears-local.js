function benchmark() {
  try {
    load("2646185526-doclist_gears_local.js");
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
  print('Time (parse-2646185526-doclist-gears-local): ' + Math.floor(1000 * elapsed/n) + ' us.');
}

run();
