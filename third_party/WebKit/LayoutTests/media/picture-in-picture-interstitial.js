function assertPictureInPictureInterstitialVisible(videoElement) {
  assert_true(isVisible(overlayPlayButton(videoElement)),
      "picture in picture interstitial should be visible");
}

function assertPictureInPictureInterstitialNotVisible(videoElement) {
  assert_false(isVisible(overlayPlayButton(videoElement)),
      "picture in picture interstitial should not be visible");
}

function pictureInPictureInterstitial(videoElement) {
  var elementId = '-internal-picture-in-picture-interstitial';
  var interstitial = mediaControlsElement(
      window.internals.shadowRoot(videoElement).firstChild,
      elementId);
  if (!interstitial)
    throw 'Failed to find picture in picture interstitial.';
  return interstitial;
}

function enablePictureInPictureForTest(t) {
  var pictureInPictureEnabledValue =
      internals.runtimeFlags.pictureInPictureEnabled;
  internals.runtimeFlags.pictureInPictureEnabled = true;

  t.add_cleanup(() => {
    internals.runtimeFlags.pictureInPictureEnabled =
        pictureInPictureEnabledValue;
  });
}