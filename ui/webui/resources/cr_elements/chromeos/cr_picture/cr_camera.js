// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'cr-camera' is a Polymer element used to take a picture from the
 * user webcam to use as a Chrome OS profile picture.
 */
(function() {

/**
 * Dimensions for camera capture.
 * @const
 */
var CAPTURE_SIZE = {width: 576, height: 576};

/**
 * Interval between frames for camera capture (milliseconds).
 * @const
 */
var CAPTURE_INTERVAL_MS = 1000 / 10;

/**
 * Duration of camera capture (milliseconds).
 * @const
 */
var CAPTURE_DURATION_MS = 1000;

Polymer({
  is: 'cr-camera',

  behaviors: [CrPngBehavior],

  properties: {
    /** Strings provided by host */
    takePhotoLabel: String,
    switchModeLabel: String,

    /**
     * True if currently in video mode.
     * @private {boolean}
     */
    videomode: {
      type: Boolean,
      value: false,
      reflectToAttribute: true,
    },

    /**
     * True when the camera is actually streaming video. May be false even when
     * the camera is present and shown, but still initializing.
     * @private {boolean}
     */
    cameraOnline_: {
      type: Boolean,
      value: false,
    },
  },

  /** @override */
  attached: function() {
    this.$.cameraVideo.addEventListener('canplay', function() {
      this.cameraOnline_ = true;
    }.bind(this));
    this.startCamera();
  },

  /** @override */
  detached: function() {
    this.stopCamera();
  },

  /**
   * Performs photo capture from the live camera stream. A 'photo-taken' event
   * will be fired as soon as captured photo is available, with the
   * 'photoDataURL' property containing the photo encoded as a data URL.
   */
  takePhoto: function() {
    if (!this.cameraOnline_)
      return;

    /** Pre-allocate all frames needed for capture. */
    var captureSize = this.getCaptureSize_();
    var numFramesToCapture =
        this.videomode ? CAPTURE_DURATION_MS / CAPTURE_INTERVAL_MS : 1;
    var allocatedFrames = [];
    for (var i = 0; i < numFramesToCapture; ++i) {
      var canvas =
          /** @type {!HTMLCanvasElement} */ (document.createElement('canvas'));
      canvas.width = captureSize.width;
      canvas.height = captureSize.height;
      var ctx =
          /** @type {!CanvasRenderingContext2D} */ (
              canvas.getContext('2d', {alpha: false}));
      // Flip frame horizontally.
      ctx.translate(captureSize.width, 0);
      ctx.scale(-1.0, 1.0);
      allocatedFrames.push(canvas);
    }

    /** Start capturing frames at an interval. */
    var capturedFrames = [];
    this.$.userImageStreamCrop.classList.add('capture');
    var interval = setInterval(function() {
      var canvas = allocatedFrames.pop();
      var ctx =
          /** @type {!CanvasRenderingContext2D} */ (
              canvas.getContext('2d', {alpha: false}));
      this.captureFrame_(this.$.cameraVideo, ctx);
      capturedFrames.push(canvas);

      /** Stop capturing frames when all allocated frames have been consumed. */
      if (allocatedFrames.length == 0) {
        this.$.userImageStreamCrop.classList.remove('capture');
        clearInterval(interval);

        /** Encode captured frames. */
        var encodedFrames = [];
        for (var i = 0; i < capturedFrames.length; ++i)
          encodedFrames.push(capturedFrames[i].toDataURL('image/png'));

        /** No need for further processing if we captured a single frame. */
        if (encodedFrames.length == 1) {
          this.fire('photo-taken', {photoDataUrl: encodedFrames[0]});
          return;
        }

        /** Create forward/backward image sequence. */
        var forwardBackwardImageSequence = [];
        for (var i = 0; i < encodedFrames.length; ++i)
          forwardBackwardImageSequence.push(encodedFrames[i]);
        for (var i = encodedFrames.length - 2; i > 0; --i)
          forwardBackwardImageSequence.push(encodedFrames[i]);

        /** Convert image sequence to animated PNG and fire 'photo-taken' event.
         */
        this.fire('photo-taken', {
          photoDataUrl: CrPngBehavior.convertImageSequenceToPng(
              forwardBackwardImageSequence)
        });
      }
    }.bind(this), CAPTURE_INTERVAL_MS);
  },

  /** Tries to start the camera stream capture. */
  startCamera: function() {
    this.stopCamera();
    this.cameraStartInProgress_ = true;

    var successCallback = function(stream) {
      if (this.cameraStartInProgress_) {
        this.$.cameraVideo.src = URL.createObjectURL(stream);
        this.cameraStream_ = stream;
      } else {
        this.stopVideoTracks_(stream);
      }
      this.cameraStartInProgress_ = false;
    }.bind(this);

    var errorCallback = function() {
      this.cameraOnline_ = false;
      this.cameraStartInProgress_ = false;
    }.bind(this);

    navigator.webkitGetUserMedia({video: true}, successCallback, errorCallback);
  },

  /** Stops the camera stream capture if it's currently active. */
  stopCamera: function() {
    this.cameraOnline_ = false;
    this.$.cameraVideo.src = '';
    if (this.cameraStream_)
      this.stopVideoTracks_(this.cameraStream_);
    // Cancel any pending getUserMedia() checks.
    this.cameraStartInProgress_ = false;
  },

  /**
   * Stops all video tracks associated with a MediaStream object.
   * @param {!MediaStream} stream
   * @private
   */
  stopVideoTracks_: function(stream) {
    var tracks = stream.getVideoTracks();
    for (var i = 0; i < tracks.length; i++)
      tracks[i].stop();
  },

  /**
   * Switch between photo and video mode.
   * @private
   */
  onTapSwitchMode_: function() {
    this.videomode = !this.videomode;
    this.fire('switch-mode', this.videomode);
  },

  /**
   * Returns capture size based on current camera mode.
   * @return {{width: number, height: number}} The capture size.
   * @private
   */
  getCaptureSize_: function() {
    /** Reduce capture size when in video mode. */
    if (this.videomode) {
      return {height: CAPTURE_SIZE.height / 2, width: CAPTURE_SIZE.width / 2};
    }
    return CAPTURE_SIZE;
  },

  /**
   * Captures a single still frame from a <video> element, placing it at the
   * current drawing origin of a canvas context.
   * @param {!HTMLVideoElement} video Video element to capture from.
   * @param {!CanvasRenderingContext2D} ctx Canvas context to draw onto.
   * @private
   */
  captureFrame_: function(video, ctx) {
    var width = video.videoWidth;
    var height = video.videoHeight;
    var captureSize = this.getCaptureSize_();
    if (width < captureSize.width || height < captureSize.height) {
      console.error(
          'Video capture size too small: ' + width + 'x' + height + '!');
    }
    var src = {};
    if (width / captureSize.width > height / captureSize.height) {
      // Full height, crop left/right.
      src.height = height;
      src.width = height * captureSize.width / captureSize.height;
    } else {
      // Full width, crop top/bottom.
      src.width = width;
      src.height = width * captureSize.height / captureSize.width;
    }
    src.x = (width - src.width) / 2;
    src.y = (height - src.height) / 2;
    ctx.drawImage(
        video, src.x, src.y, src.width, src.height, 0, 0, captureSize.width,
        captureSize.height);
  },
});

})();
