/**
 * Assert AudioWorklet Runtime flag is enabled.
 *
 * The content_shell driven by run-webkit-tests.py is supposed to enable all the
 * experimental web platform features.
 *
 * We also want to run the test on the browser. So we check both cases for
 * the content shell and the browser.
 */
function assertAudioWorklet() {
  let offlineContext = new OfflineAudioContext(1, 1, 44100);
  if ((Boolean(window.internals) &&
       Boolean(window.internals.runtimeFlags.audioWorkletEnabled)) ||
      offlineContext.audioWorklet instanceof AudioWorklet) {
    return;
  }

  throw new Error('AudioWorklet is not enabled.');
}
