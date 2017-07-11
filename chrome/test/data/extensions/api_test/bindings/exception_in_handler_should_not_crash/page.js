// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A named function that throws the error, used to verify that the error message
// has a stack trace that contains the relevant stack frames.
function throwNewError(message) {
  console.warn('Throwing error');
  throw new Error(message)
}

chrome.test.runTests([
  function tabsCreateThrowsError() {
    console.warn('Setting');
    chrome.test.setExceptionHandler(function(message, exception) {
      console.warn('Got exception');
      console.warn('stack: ' + (exception && exception.stack));
      chrome.test.assertTrue(exception.stack.indexOf('throwNewError') >= 0);
      chrome.test.assertEq('tata', exception.message);
      chrome.test.succeed();
    });
    chrome.tabs.create({}, function() {
      console.warn('In handler');
      throwNewError('tata');
    });
  },

  function tabsOnCreatedThrowsError() {
    var listener = function() {
      throwNewError('hi');
    };
    chrome.test.setExceptionHandler(function(message, exception) {
      chrome.test.assertTrue(exception.stack.indexOf('throwNewError') >= 0);
      chrome.tabs.onCreated.removeListener(listener);
      chrome.test.succeed();
    });
    chrome.tabs.onCreated.addListener(listener);
    chrome.tabs.create({});
  },

  function permissionsGetAllThrowsError() {
    // permissions.getAll has a custom callback, as do many other methods, but
    // this is easy to call.
    chrome.test.setExceptionHandler(function(message, exception) {
      chrome.test.assertTrue(exception.stack.indexOf('throwNewError') >= 0);
      chrome.test.assertEq('boom', exception.message);
      chrome.test.succeed();
    });
    chrome.permissions.getAll(function() {
      throwNewError('boom');
    });
  }
]);
