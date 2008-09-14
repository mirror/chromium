// The Parsy benchmark parses real JavaScript code for real Google web
// applications. You probably have to run it from the directory that
// contains the benchmark JS resources.

const kBenchmarks = [
    "1955018496-en-scroll.js",
    "2646185526-doclist_gears_local.js",
    "2903028364-trix_core.js",
    "3012310422-ContactUi.js",
    "lh_experimental_en_US.js"
];


function Run(benchmark) {
  try {
    load(benchmark);
  } catch (e) {
    // Do nothing.
  }
}


function RunBenchmarks(iterations) {
  for (var j = kBenchmarks.length - 1; j >= 0; j--) {
    var benchmark = kBenchmarks[j];
    if (this.gc) gc();
    var start = new Date();
    for (var i = 0; i < iterations; i++) Run(benchmark);
    var end = new Date();
    print('Time (' + benchmark + '): ' + (end - start) + ' ms.');
  }
}


RunBenchmarks(10);
