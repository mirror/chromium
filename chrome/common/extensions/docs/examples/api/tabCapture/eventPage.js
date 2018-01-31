// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The window (tab) opened and navigated to receiver.html.
var receiver = null;
var prefer_stream_id = true;
// Open a new window of receiver.html when browser action icon is clicked.
chrome.browserAction.onClicked.addListener(function(tab) {
  chrome.tabCapture.capture(
    {video: true, audio: true,
     videoConstraints: {
       mandatory: {
         // If minWidth/Height have the same aspect ratio (e.g., 16:9) as
         // maxWidth/Height, the implementation will letterbox/pillarbox as
         // needed. Otherwise, set minWidth/Height to 0 to allow output video
         // to be of any arbitrary size.
         minWidth: 16,
         minHeight: 9,
         maxWidth: 854,
         maxHeight: 480,
         maxFrameRate: 60,  // Note: Frame rate is variable (0 <= x <= 60).
       },
     },
     streamId: prefer_stream_id,
    },
    function(response) {
      if (!response) {
        console.error('Error starting tab capture: ' +
                      (chrome.runtime.lastError.message || 'UNKNOWN'));
        return;
      }

      if (!prefer_stream_id) {
        console.log("Get stream and display directly.");
        if (receiver != null) {
          receiver.close();
        }
        receiver = window.open('receiver.html');
        receiver.currentStream = response;
      } else {
        console.log("Get tab id and request user media.");
        var options = {};
        if (response.audioConstraints)
          options.audio = response.audioConstraints;
        if (response.videoConstraints)
          options.video = response.videoConstraints;
        try {
          navigator.webkitGetUserMedia(
            options,
            function onSuccess(stream) {
              if (receiver != null) {
                receiver.close();
              }
              receiver = window.open('receiver.html');
              receiver.currentStream = stream;
            },
            function onError(error) {
              console.log("brave: gUM error.");
            });
        } catch (error) {
          runCallbackWithLastError(
            name, getErrorMessage(error, "Invalid argument(s)."), request.stack,
            function() { callback(null); });
        }
      }
    }
  );
});
