// Because of a bug in the heuristics used to figure out when to
// convert a slow-case elements backing storage to fast case, the
// following took a very long time.
//
// This test passes if it finishes almost immediately.
for (var repetitions = 0; repetitions < 20; repetitions++) {
  var stride = 500;
  var array = [];
  for (var i = 0; i < 1000; i++) {
    array[i * stride] = i;
  }
}


