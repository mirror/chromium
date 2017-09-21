/**
 * Tests autoplay on a page.
 */

function tearDown(result) {
  window.internals.settings.setAutoplayPolicy('no-user-gesture-required');
  testRunner.notifyDone();

  shouldBeUndefined(result);
  finishJSTest();
}

function runVideoTest() {
  console.log("Testing on: " + window.location.href);

  var video = document.createElement('video');
  video.src = findMediaFile('video', 'resources/test');
  video.play().then(tearDown, tearDown);

  var jsTestIsAsync = true;
}

if (window.testRunner) {
  window.internals.settings.setAutoplayPolicy('document-user-activation-required');
  testRunner.dumpAsText();
  runVideoTest();
  testRunner.dumpBackForwardList();
}
