/**
 * Tests autoplay on a page.
 */

function tearDown(result) {
  // Reset the flag state.
  window.internals.settings.setAutoplayPolicy('no-user-gesture-required');

  testRunner.notifyDone();
  shouldBeUndefined(result);
  finishJSTest();
}

function fail(result) {
  console.log(result.message);
  tearDown();
}

function runVideoTest() {
  console.log("Testing on: " + window.location.href);

  var video = document.createElement('video');
  video.src = findMediaFile('video', '/media-resources/content/test');
  video.play().then(tearDown, fail);

  var jsTestIsAsync = true;
}

function simulateMouseDown() {
  chrome.gpuBenchmarking.pointerActionSequence([
      {"source": "mouse",
       "actions": [
       { "name": "pointerDown", "x": 0, "y": 0 },
       { "name": "pointerUp" } ]}]);
}

function simulateFrameClick() {
  const frame = document.getElementsByTagName('iframe')[0];
  const rect = frame.getBoundingClientRect();

  chrome.gpuBenchmarking.pointerActionSequence([
      {"source": "mouse",
       "actions": [
       { "name": "pointerDown", "x": rect.left, "y": rect.top },
       { "name": "pointerUp" } ]}]);
}

function runTest() {
  if (!window.testRunner)
    return;

  testRunner.waitUntilDone();
}

window.addEventListener('load', () => {
  if (!window.testRunner)
    return;

  // Setup the flags before the test is run.
  window.internals.settings.setAutoplayPolicy('document-user-activation-required');

  testRunner.dumpAsText();
  runVideoTest();
}, { once: true });
