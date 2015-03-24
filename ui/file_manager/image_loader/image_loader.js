// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Loads and resizes an image.
 * @constructor
 */
function ImageLoader() {
  /**
   * Persistent cache object.
   * @type {Cache}
   * @private
   */
  this.cache_ = new Cache();

  /**
   * Manages pending requests and runs them in order of priorities.
   * @type {Scheduler}
   * @private
   */
  this.scheduler_ = new Scheduler();

  /**
   * Piex loader for RAW images.
   * @private {!PiexLoader}
   */
  this.piexLoader_ = new PiexLoader();

  // Grant permissions to all volumes, initialize the cache and then start the
  // scheduler.
  chrome.fileManagerPrivate.getVolumeMetadataList(function(volumeMetadataList) {
    // Listen for mount events, and grant permissions to volumes being mounted.
    chrome.fileManagerPrivate.onMountCompleted.addListener(
        function(event) {
          if (event.eventType === 'mount' && event.status === 'success') {
            chrome.fileSystem.requestFileSystem(
                {volumeId: event.volumeMetadata.volumeId}, function() {});
          }
        });
    var initPromises = volumeMetadataList.map(function(volumeMetadata) {
      var requestPromise = new Promise(function(callback) {
        chrome.fileSystem.requestFileSystem(
            {volumeId: volumeMetadata.volumeId},
            callback);
      });
      return requestPromise;
    });
    initPromises.push(new Promise(function(resolve, reject) {
      this.cache_.initialize(resolve);
    }.bind(this)));

    // After all initialization promises are done, start the scheduler.
    Promise.all(initPromises).then(this.scheduler_.start.bind(this.scheduler_));
  }.bind(this));

  // Listen for incoming requests.
  chrome.runtime.onMessageExternal.addListener(
      function(request, sender, sendResponse) {
        if (ImageLoader.ALLOWED_CLIENTS.indexOf(sender.id) !== -1) {
          // Sending a response may fail if the receiver already went offline.
          // This is not an error, but a normal and quite common situation.
          var failSafeSendResponse = function(response) {
            try {
              sendResponse(response);
            }
            catch (e) {
              // Ignore the error.
            }
          };
          return this.onMessage_(sender.id,
                                 /** @type {LoadImageRequest} */ (request),
                                 failSafeSendResponse);
        }
      }.bind(this));
}

/**
 * List of extensions allowed to perform image requests.
 *
 * @const
 * @type {Array.<string>}
 */
ImageLoader.ALLOWED_CLIENTS = [
  'hhaomjibdihmijegdhdafkllkbggdgoj',  // File Manager's extension id.
  'nlkncpkkdoccmpiclbokaimcnedabhhm'  // Gallery extension id.
];

/**
 * Handles a request. Depending on type of the request, starts or stops
 * an image task.
 *
 * @param {string} senderId Sender's extension id.
 * @param {LoadImageRequest} request Request message as a hash array.
 * @param {function(Object)} callback Callback to be called to return response.
 * @return {boolean} True if the message channel should stay alive until the
 *     callback is called.
 * @private
 */
ImageLoader.prototype.onMessage_ = function(senderId, request, callback) {
  var requestId = senderId + ':' + request.taskId;
  if (request.cancel) {
    // Cancel a task.
    this.scheduler_.remove(requestId);
    return false;  // No callback calls.
  } else {
    // Create a request task and add it to the scheduler (queue).
    var requestTask = new Request(
        requestId, this.cache_, this.piexLoader_, request, callback);
    this.scheduler_.add(requestTask);
    return true;  // Request will call the callback.
  }
};

/**
 * Returns the singleton instance.
 * @return {ImageLoader} ImageLoader object.
 */
ImageLoader.getInstance = function() {
  if (!ImageLoader.instance_)
    ImageLoader.instance_ = new ImageLoader();
  return ImageLoader.instance_;
};

/**
 * Checks if the options contain any image processing.
 *
 * @param {number} width Source width.
 * @param {number} height Source height.
 * @param {Object} options Resizing options as a hash array.
 * @return {boolean} True if yes, false if not.
 */
ImageLoader.shouldProcess = function(width, height, options) {
  var targetDimensions = ImageLoader.resizeDimensions(width, height, options);

  // Dimensions has to be adjusted.
  if (targetDimensions.width != width || targetDimensions.height != height)
    return true;

  // Orientation has to be adjusted.
  if (options.orientation)
    return true;

  // No changes required.
  return false;
};

/**
 * Calculates dimensions taking into account resize options, such as:
 * - scale: for scaling,
 * - maxWidth, maxHeight: for maximum dimensions,
 * - width, height: for exact requested size.
 * Returns the target size as hash array with width, height properties.
 *
 * @param {number} width Source width.
 * @param {number} height Source height.
 * @param {Object} options Resizing options as a hash array.
 * @return {Object} Dimensions, eg. {width: 100, height: 50}.
 */
ImageLoader.resizeDimensions = function(width, height, options) {
  var sourceWidth = width;
  var sourceHeight = height;

  // Flip dimensions for odd orientation values: 1 (90deg) and 3 (270deg).
  if (options.orientation && options.orientation % 2) {
    sourceWidth = height;
    sourceHeight = width;
  }

  var targetWidth = sourceWidth;
  var targetHeight = sourceHeight;

  if ('scale' in options) {
    targetWidth = sourceWidth * options.scale;
    targetHeight = sourceHeight * options.scale;
  }

  if (options.maxWidth && targetWidth > options.maxWidth) {
    var scale = options.maxWidth / targetWidth;
    targetWidth *= scale;
    targetHeight *= scale;
  }

  if (options.maxHeight && targetHeight > options.maxHeight) {
    var scale = options.maxHeight / targetHeight;
    targetWidth *= scale;
    targetHeight *= scale;
  }

  if (options.width)
    targetWidth = options.width;

  if (options.height)
    targetHeight = options.height;

  targetWidth = Math.round(targetWidth);
  targetHeight = Math.round(targetHeight);

  return {width: targetWidth, height: targetHeight};
};

/**
 * Performs resizing of the source image into the target canvas.
 *
 * @param {HTMLCanvasElement|Image} source Source image or canvas.
 * @param {HTMLCanvasElement} target Target canvas.
 * @param {Object} options Resizing options as a hash array.
 */
ImageLoader.resize = function(source, target, options) {
  var targetDimensions = ImageLoader.resizeDimensions(
      source.width, source.height, options);

  // Default orientation is 0deg.
  var orientation = options.orientation || new ImageOrientation(1, 0, 0, 1);
  var size = orientation.getSizeAfterCancelling(
      targetDimensions.width, targetDimensions.height);
  target.width = size.width;
  target.height = size.height;

  var targetContext = target.getContext('2d');
  targetContext.save();
  orientation.cancelImageOrientation(
      targetContext, targetDimensions.width, targetDimensions.height);
  targetContext.drawImage(
      source,
      0, 0, source.width, source.height,
      0, 0, targetDimensions.width, targetDimensions.height);
  targetContext.restore();
};
