// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'cr-picture-preview' is a Polymer element used to show either a profile
 * picture or a camera image preview.
 */

Polymer({
  is: 'cr-picture-preview',

  properties: {
    /**
     * Image source to show when |cameraActive| is false.
     * @type {boolean}
     */
    imageSrc: String,

    /**
     * Whetner the camera should be used for the preview image.
     * @type {boolean}
     */
    cameraActive: {
      type: Boolean,
      observer: 'cameraActiveChanged_',
    },

    /**
     * Whether to show the 'discard' button.
     * @type {boolean}
     */
    showDiscard: Boolean,

    /** Strings provided by host */
    discardImageLabel: String,
    flipPhotoLabel: String,
    previewAltText: String,
    takePhotoLabel: String,
  },

  takePhoto: function() {
    var camera = /** @type {?CrCameraElement} */ (this.$$('#camera'));
    if (camera)
      camera.takePhoto();
  },

  /** @private */
  cameraActiveChanged_: function() {
    var camera = /** @type {?CrCameraElement} */ (this.$$('#camera'));
    if (!camera)
      return;  // Camera will be started when attached.
    if (this.cameraActive)
      camera.startCamera();
    else
      camera.stopCamera();
  },
  /**
   * @return {boolean}
   * @private
   */
  showImagePreview_: function() {
    return !this.cameraActive && !!this.imageSrc;
  },

  /** @private */
  onTapDiscardImage_: function() {
    this.fire('discard-image');
  },
});
