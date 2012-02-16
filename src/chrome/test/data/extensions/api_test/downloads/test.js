// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// downloads api test
// browser_tests.exe --gtest_filter=DownloadsApiTest.Downloads

var downloads = chrome.experimental.downloads;

chrome.test.getConfig(function(testConfig) {
  function getURL(path) {
    return 'http://localhost:' + testConfig.testServer.port + '/' + path;
  }

  var nextId = 0;
  function getNextId() {
    return nextId++;
  }

  // There can be only one assertThrows per test function.  This should be
  // called as follows:
  //
  //   assertThrows(exceptionMessage, function, [arg1, [arg2, ... ]])
  //
  // ... where |exceptionMessage| is the expected exception message.
  // |arg1|, |arg2|, etc.. are arguments to |function|.
  function assertThrows(exceptionMessage, func) {
    var args = Array.prototype.slice.call(arguments, 2);
    try {
      args.push(function() {
        // Don't use chrome.test.callbackFail because it requires the
        // callback to be called.
        chrome.test.fail('Failed to throw exception (' +
                         exceptionMessage + ')');
      });
      func.apply(null, args);
    } catch (exception) {
      chrome.test.assertEq(exceptionMessage, exception.message);
      chrome.test.succeed();
    }
  }

  // The "/slow" handler waits a specified amount of time before returning a
  // safe file. Specify zero seconds to return quickly.
  var SAFE_FAST_URL = getURL('slow?0');

  var NEVER_FINISH_URL = getURL('download-known-size');

  // This URL should only work with the POST method and a request body
  // containing 'BODY'.
  var POST_URL = getURL('files/post/downloads/a_zip_file.zip?' +
                        'expected_body=BODY');

  // This URL should only work with headers 'Foo: bar' and 'Qx: yo'.
  var HEADERS_URL = getURL('files/downloads/a_zip_file.zip?' +
                           'expected_headers=Foo:bar&expected_headers=Qx:yo');

  chrome.test.runTests([
    // TODO(benjhayden): Test onErased using remove().

    // TODO(benjhayden): Sub-directories depend on http://crbug.com/109443
    // TODO(benjhayden): Windows slashes.
    // function downloadSubDirectoryFilename() {
    //   var downloadId = getNextId();
    //   var callbackCompleted = chrome.test.callbackAdded();
    //   function myListener(delta) {
    //     if ((delta.id != downloadId) ||
    //         !delta.filename ||
    //         (delta.filename.new.indexOf('/foo/slow') == -1))
    //       return;
    //     downloads.onChanged.removeListener(myListener);
    //     callbackCompleted();
    //   }
    //   downloads.onChanged.addListener(myListener);
    //   downloads.download(
    //       {'url': SAFE_FAST_URL, 'filename': 'foo/slow'},
    //       chrome.test.callback(function(id) {
    //         chrome.test.assertEq(downloadId, id);
    //       }));
    // },

    function downloadSimple() {
      // Test that we can begin a download.
      var downloadId = getNextId();
      console.log(downloadId);
      downloads.download(
          {'url': SAFE_FAST_URL},
          chrome.test.callback(function(id) {
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadPostSuccess() {
      // Test the |method| download option.
      var downloadId = getNextId();
      console.log(downloadId);
      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides completion.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
        console.log(downloadId);
        downloads.search({id: downloadId},
                          chrome.test.callback(function(items) {
          console.log(downloadId);
          chrome.test.assertEq(1, items.length);
          chrome.test.assertEq(downloadId, items[0].id);
          var EXPECTED_SIZE = 164;
          chrome.test.assertEq(EXPECTED_SIZE, items[0].totalBytes);
          chrome.test.assertEq(EXPECTED_SIZE, items[0].fileSize);
          chrome.test.assertEq(EXPECTED_SIZE, items[0].bytesReceived);
        }));
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': POST_URL,
           'method': 'POST',
           'filename': downloadId + '.txt',
           'body': 'BODY'},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadPostWouldFailWithoutMethod() {
      // Test that downloadPostSuccess would fail if the resource requires the
      // POST method, and chrome fails to propagate the |method| parameter back
      // to the server. This tests both that testserver.py does not succeed when
      // it should fail, and this tests how the downloads extension api exposes
      // the failure to extensions.
      var downloadId = getNextId();
      console.log(downloadId);

      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides interruption.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
        console.log(downloadId);
        // TODO(benjhayden): Change COMPLETE to INTERRUPTED after
        // http://crbug.com/112342
        downloads.search({id: downloadId},
                          chrome.test.callback(function(items) {
          console.log(downloadId);
          chrome.test.assertEq(1, items.length);
          chrome.test.assertEq(downloadId, items[0].id);
          chrome.test.assertEq(0, items[0].totalBytes);
        }));
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': POST_URL,
           'filename': downloadId + '.txt',  // Prevent 'file' danger.
           'body': 'BODY'},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadPostWouldFailWithoutBody() {
      // Test that downloadPostSuccess would fail if the resource requires the
      // POST method and a request body, and chrome fails to propagate the
      // |body| parameter back to the server. This tests both that testserver.py
      // does not succeed when it should fail, and this tests how the downloads
      // extension api exposes the failure to extensions.
      var downloadId = getNextId();
      console.log(downloadId);

      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides interruption.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
        console.log(downloadId);
        // TODO(benjhayden): Change COMPLETE to INTERRUPTED after
        // http://crbug.com/112342
        downloads.search({id: downloadId},
                          chrome.test.callback(function(items) {
          console.log(downloadId);
          chrome.test.assertEq(1, items.length);
          chrome.test.assertEq(downloadId, items[0].id);
          chrome.test.assertEq(0, items[0].totalBytes);
        }));
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': POST_URL,
           'filename': downloadId + '.txt',  // Prevent 'file' danger.
           'method': 'POST'},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadHeadersSuccess() {
      // Test the |header| download option.
      var downloadId = getNextId();
      console.log(downloadId);
      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides completion.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
        console.log(downloadId);
        downloads.search({id: downloadId},
                          chrome.test.callback(function(items) {
          console.log(downloadId);
          chrome.test.assertEq(1, items.length);
          chrome.test.assertEq(downloadId, items[0].id);
          var EXPECTED_SIZE = 164;
          chrome.test.assertEq(EXPECTED_SIZE, items[0].totalBytes);
          chrome.test.assertEq(EXPECTED_SIZE, items[0].fileSize);
          chrome.test.assertEq(EXPECTED_SIZE, items[0].bytesReceived);
        }));
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': HEADERS_URL,
           'filename': downloadId + '.txt',  // Prevent 'file' danger.
           'headers': [{'name': 'Foo', 'value': 'bar'},
                       {'name': 'Qx', 'value': 'yo'}]},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadHeadersWouldFail() {
      // Test that downloadHeadersSuccess() would fail if the resource requires
      // the headers, and chrome fails to propagate them back to the server.
      // This tests both that testserver.py does not succeed when it should
      // fail as well as how the downloads extension api exposes the
      // failure to extensions.
      var downloadId = getNextId();
      console.log(downloadId);

      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides interruption.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
        console.log(downloadId);
        // TODO(benjhayden): Change COMPLETE to INTERRUPTED after
        // http://crbug.com/112342
        downloads.search({id: downloadId},
                          chrome.test.callback(function(items) {
          console.log(downloadId);
          chrome.test.assertEq(1, items.length);
          chrome.test.assertEq(downloadId, items[0].id);
          chrome.test.assertEq(0, items[0].totalBytes);
        }));
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': HEADERS_URL},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadInterrupted() {
      // Test that cancel()ing an in-progress download causes its state to
      // transition to interrupted, and test that that state transition is
      // detectable by an onChanged event listener.
      // TODO(benjhayden): Test other sources of interruptions such as server
      // death.
      var downloadId = getNextId();
      console.log(downloadId);

      var createdCompleted = chrome.test.callbackAdded();
      function createdListener(createdItem) {
        console.log(createdItem.id);
        // Ignore onCreated events for any download besides our own.
        if (createdItem.id != downloadId)
          return;
        console.log(downloadId);
        // TODO(benjhayden) Move this cancel() into the download() callback
        // after ensuring that DownloadItems are created before that callback
        // is fired.
        downloads.cancel(downloadId, chrome.test.callback(function() {
          console.log(downloadId);
        }));
        downloads.onCreated.removeListener(createdListener);
        createdCompleted();
      }
      downloads.onCreated.addListener(createdListener);

      var changedCompleted = chrome.test.callbackAdded();
      function changedListener(delta) {
        console.log(delta.id);
        // Ignore onChanged events for downloads besides our own, or events that
        // signal any change besides interruption.
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_INTERRUPTED) ||
            !delta.error ||
            (delta.error.new != 40))
          return;
        console.log(downloadId);
        downloads.onChanged.removeListener(changedListener);
        changedCompleted();
      }
      downloads.onChanged.addListener(changedListener);

      downloads.download(
          {'url': NEVER_FINISH_URL},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadOnChanged() {
      // Test that download completion is detectable by an onChanged event
      // listener.
      var downloadId = getNextId();
      console.log(downloadId);
      var callbackCompleted = chrome.test.callbackAdded();
      function myListener(delta) {
        console.log(delta.id);
        if ((delta.id != downloadId) ||
            !delta.state ||
            (delta.state.new != downloads.STATE_COMPLETE))
          return;
          console.log(downloadId);
        downloads.onChanged.removeListener(myListener);
        callbackCompleted();
      }
      downloads.onChanged.addListener(myListener);
      downloads.download(
        {"url": SAFE_FAST_URL},
        chrome.test.callback(function(id) {
          console.log(downloadId);
          chrome.test.assertEq(downloadId, id);
      }));
    },

    function downloadFilename() {
      // Test that we can suggest a filename for a new download, and test that
      // we can detect filename changes with an onChanged event listener.
      var FILENAME = 'owiejtoiwjrfoiwjroiwjroiwjroiwjrfi';
      var downloadId = getNextId();
      console.log(downloadId);
      var callbackCompleted = chrome.test.callbackAdded();
      function myListener(delta) {
        console.log(delta.id);
        if ((delta.id != downloadId) ||
            !delta.filename ||
            (delta.filename.new.indexOf(FILENAME) == -1))
          return;
        console.log(downloadId);
        downloads.onChanged.removeListener(myListener);
        callbackCompleted();
      }
      downloads.onChanged.addListener(myListener);
      downloads.download(
          {'url': SAFE_FAST_URL, 'filename': FILENAME},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadOnCreated() {
      // Test that the onCreated event fires when we start a download.
      var downloadId = getNextId();
      console.log(downloadId);
      var createdCompleted = chrome.test.callbackAdded();
      function createdListener(item) {
        console.log(item.id);
        if (item.id != downloadId)
          return;
        console.log(downloadId);
        createdCompleted();
        downloads.onCreated.removeListener(createdListener);
      };
      downloads.onCreated.addListener(createdListener);
      downloads.download(
          {'url': SAFE_FAST_URL},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function downloadInvalidFilename() {
      // Test that we disallow invalid filenames for new downloads.
      downloads.download(
          {'url': SAFE_FAST_URL, 'filename': '../../../../../etc/passwd'},
          chrome.test.callbackFail(downloads.ERROR_GENERIC));
    },

    function downloadEmpty() {
      assertThrows(('Invalid value for argument 1. Property \'url\': ' +
                    'Property is required.'),
                   downloads.download, {});
    },

    function downloadInvalidSaveAs() {
      assertThrows(('Invalid value for argument 1. Property \'saveAs\': ' +
                    'Expected \'boolean\' but got \'string\'.'),
                   downloads.download,
                   {'url': SAFE_FAST_URL, 'saveAs': 'GOAT'});
    },

    function downloadInvalidHeadersOption() {
      assertThrows(('Invalid value for argument 1. Property \'headers\': ' +
                    'Expected \'array\' but got \'string\'.'),
                   downloads.download,
                   {'url': SAFE_FAST_URL, 'headers': 'GOAT'});
    },

    function downloadInvalidURL() {
      // Test that download() requires a valid url.
      downloads.download(
          {'url': 'foo bar'},
          chrome.test.callbackFail(downloads.ERROR_INVALID_URL));
    },

    function downloadInvalidMethod() {
      assertThrows(('Invalid value for argument 1. Property \'method\': ' +
                    'Value must be one of: [GET, POST].'),
                   downloads.download,
                   {'url': SAFE_FAST_URL, 'method': 'GOAT'});
    },

    function downloadInvalidHeader() {
      // Test that download() disallows setting the Cookie header.
      downloads.download(
          {'url': SAFE_FAST_URL,
           'headers': [{ 'name': 'Cookie', 'value': 'fake'}]
        },
        chrome.test.callbackFail(downloads.ERROR_GENERIC));
    },

    function downloadGetFileIconInvalidOptions() {
      assertThrows(('Invalid value for argument 2. Property \'cat\': ' +
                    'Unexpected property.'),
                   downloads.getFileIcon,
                   -1, {cat: 'mouse'});
    },

    function downloadGetFileIconInvalidSize() {
      assertThrows(('Invalid value for argument 2. Property \'size\': ' +
                    'Value must be one of: [16, 32].'),
                   downloads.getFileIcon, -1, {size: 31});
    },

    function downloadGetFileIconInvalidId() {
      downloads.getFileIcon(-42, {size: 32},
        chrome.test.callbackFail(downloads.ERROR_INVALID_OPERATION));
    },

    function downloadPauseInvalidId() {
      downloads.pause(-42, chrome.test.callbackFail(
            downloads.ERROR_INVALID_OPERATION));
    },

    function downloadPauseInvalidType() {
      assertThrows(('Invalid value for argument 1. Expected \'integer\' ' +
                    'but got \'string\'.'),
                   downloads.pause,
                   'foo');
    },

    function downloadResumeInvalidId() {
      downloads.resume(-42, chrome.test.callbackFail(
            downloads.ERROR_INVALID_OPERATION));
    },

    function downloadResumeInvalidType() {
      assertThrows(('Invalid value for argument 1. Expected \'integer\' ' +
                    'but got \'string\'.'),
                   downloads.resume,
                   'foo');
    },

    function downloadCancelInvalidId() {
      // Canceling a non-existent download is not considered an error.
      downloads.cancel(-42, chrome.test.callback(function() {
        console.log('');
      }));
    },

    function downloadCancelInvalidType() {
      assertThrows(('Invalid value for argument 1. Expected \'integer\' ' +
                    'but got \'string\'.'),
                   downloads.cancel, 'foo');
    },

    function downloadNoComplete() {
      // This is used partly to test cleanUp.
      var downloadId = getNextId();
      console.log(downloadId);
      downloads.download(
          {'url': NEVER_FINISH_URL},
          chrome.test.callback(function(id) {
            console.log(downloadId);
            chrome.test.assertEq(downloadId, id);
          }));
    },

    function cleanUp() {
      // cleanUp must come last. It clears out all in-progress downloads
      // so the browser can shutdown cleanly.
      console.log(nextId);
      function makeCallback(id) {
        return function() {
          console.log(id);
        }
      }
      for (var id = 0; id < nextId; ++id) {
        downloads.cancel(id, chrome.test.callback(makeCallback(id)));
      }
    }
  ]);
});
