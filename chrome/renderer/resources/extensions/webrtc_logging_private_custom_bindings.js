// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the webrtcLoggingPrivate API.

var binding = apiBridge ||
              require('binding').Binding.create('webrtcLoggingPrivate');

var getBindDirectoryEntryCallback =
    require('fileEntryBindingUtil').getBindDirectoryEntryCallback;

binding.registerCustomHook(function(binding, id, contextType) {
  var apiFunctions = binding.apiFunctions;

  // There are 2 ways these APIs get called.
  //  1. From a whitelisted component extension on behalf page with the
  //     appropriate origin via a message from that page . In this case,
  //     the guestProcessId is on the message received by the component
  //     extension, and the extension can pass that in on the RequestInfo param.
  //  2. From a whitelisted app that hosts a page in a webview. In this case,
  //     the app passes the appropriate webview in on the RequestInfo param.
  //
  // The following RequestInfo processing is done to accommodate case 2.
  function populateGuestProcessIdFromWebview (functionName) {
    apiFunctions.setUpdateArgumentsPostValidate(
        functionName, function () {
          var args = $Array.slice(arguments);
          var requestInfo = args[0];
          var webview = requestInfo.webview;
          if (webview) {
            if (requestInfo.guestProcessId != null) {
              throw new Error('Cannot specify guestProcessId and webview');
            }
            if (!webview.tagName || webview.tagName != 'WEBVIEW') {
              throw new Error(
                  '<webview> element expected for RequestInfo.webview');
            }
            var webViewImpl = privates(webview).internal;
            requestInfo.guestProcessId= webViewImpl.processId;
            delete requestInfo.webview;
          }
          return args;
        });
  }

  $Array.forEach([
    'setMetaData',
    'start',
    'setUploadOnRenderClose',
    'stop',
    'store',
    'uploadStored',
    'upload',
    'discard',
    'startRtpDump',
    'stopRtpDump',
    'startAudioDebugRecordings',
    'stopAudioDebugRecordings',
    'startWebRtcEventLogging',
    'stopWebRtcEventLogging',
  ], populateGuestProcessIdFromWebview);
  apiFunctions.setCustomCallback('getLogsDirectory',
                                 getBindDirectoryEntryCallback());
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
