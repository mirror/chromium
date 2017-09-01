function runImageDecoderPerfTests(imageFile, testDescription) {
  var isDone = false;
  var iteration = 0;

  function runTest() {
    var image = new Image();

    // When all the data is available...
    image.onload = function() {
      PerfTestRunner.addRunTestStartMarker();
      var startTime = PerfTestRunner.now();

      // Issue a decode command
      image.decode().then(function () {
        PerfTestRunner.measureValueAsync(PerfTestRunner.now() - startTime);
        PerfTestRunner.addRunTestEndMarker();

        // If we have more iterations left
        if (!isDone) {
          iteration++;
          runTest();
        }
      });
    }

    // Begin fetching the data
    image.src = imageFile + "?" + iteration;
  }

  window.onload = function () {
    PerfTestRunner.startMeasureValuesAsync({
      unit: "ms",
      done: function () {
        isDone = true;
      },
      run: function () {
        runTest();
      },
      iterationCount: 20,
      description: testDescription,
      tracingCategories: 'blink',
      traceEventsToMeasure: ['ImageFrameGenerator::decode'],
    });
  };
}
