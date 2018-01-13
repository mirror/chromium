function assertPictureInPictureButtonVisible(videoElement)
{
  assert_true(isVisible(pictureInPictureButton(videoElement)),
      "picture in picture button should be visible");
}

function assertPictureInPictureButtonNotVisible(videoElement)
{
  assert_false(isVisible(pictureInPictureButton(videoElement)),
      "picture in picture button should not be visible");
}

function pictureInPictureButton(videoElement)
{
  var elementId = '-internal-media-controls-picture-in-picture-button';
  var button = mediaControlsElement(
      window.internals.shadowRoot(videoElement).firstChild,
      elementId);
  if (!button)
    throw 'Failed to find picture in picture button.';
  return button;
}

// ---------

function assertPictureInPictureInterstitialVisible(videoElement)
{
  assert_true(isVisible(pictureInPictureInterstitial(videoElement)),
      "picture in picture interstitial should be visible");
}

function assertPictureInPictureInterstitialNotVisible(videoElement)
{
  assert_false(isVisible(pictureInPictureInterstitial(videoElement)),
      "picture in picture interstitial should not be visible");
}

function pictureInPictureInterstitial(videoElement)
{
  var elementId = '-internal-picture-in-picture-interstitial';
  var interstitial = mediaControlsElement(
      window.internals.shadowRoot(videoElement).firstChild,
      elementId);
  if (!interstitial)
    throw 'Failed to find picture in picture interstitial.';
  return interstitial;
}

function enablePictureInPictureForTest(t)
{
  var pictureInPictureEnabledValue =
      internals.runtimeFlags.pictureInPictureEnabled;
  internals.runtimeFlags.pictureInPictureEnabled = true;

  t.add_cleanup(() => {
    internals.runtimeFlags.pictureInPictureEnabled =
        pictureInPictureEnabledValue;
  });
}