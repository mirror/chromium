// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Test for the "paste in directory" menu item
 */
testcase.pasteIntoDirectoryInFileContextMenu = function() {
  const entrySet = [ENTRIES.directoryA, ENTRIES.hello];
  var appId;
  StepsRunner.run([
    function() {
      addEntries(['local'], entrySet, this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      openNewWindow(null, RootPath.DOWNLOADS).then(this.next);
    },
    function(inAppId) {
      appId = inAppId;
      remoteCall.waitForElement(appId, '#detail-table').then(this.next);
    },
    // Wait for the expected files to appear in the file list.
    function() {
      remoteCall.waitForFiles(appId, TestEntryInfo.getExpectedRows(entrySet))
          .then(this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#selection-menu-button')
          .then(this.next);
    },

    // 1. Selecting a single file
    function(result) {
      remoteCall.callRemoteTestUtil(
          'toggleSelectFile', appId, [ENTRIES.hello.nameText], this.next);
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#selection-menu-button'], this.next);
    },
    // Wait for menu to appear.
    // The command is still hidden because the selection is not a directory.
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall
          .waitForElement(
              appId,
              '#file-context-menu:not([hidden])' +
                  ' cr-menu-item[command=\'#paste-into-folder\'][hidden]')
          .then(this.next);
    },
    function() {
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#file-context-menu[hidden]')
          .then(this.next);
    },
    function() {
      // clear selection
      remoteCall.callRemoteTestUtil(
          'toggleSelectFile', appId, [ENTRIES.hello.nameText], this.next);
    },

    // 2. Selecting a directory
    function(result) {
      remoteCall.callRemoteTestUtil(
          'toggleSelectFile', appId, [ENTRIES.directoryA.nameText], this.next);
      console.log('1');
    },
    function(result) {
      chrome.test.assertTrue(result);
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#selection-menu-button'], this.next);
      console.log(2);
    },
    // Wait for menu to appear.
    // The command is still hidden because the selection is not a directory.
    function() {
      console.log('3');
      remoteCall
          .waitForElement(
              appId,
              '#file-context-menu:not([hidden]) cr-menu-item' +
                  '[command=\'#paste-into-folder\']:not([hidden])')
          .then(this.next);
    },
    function() {
      console.log('4');
      remoteCall.callRemoteTestUtil(
          'fakeMouseClick', appId, ['#file-list'], this.next);
    },
    function() {
      remoteCall.waitForElement(appId, '#file-context-menu[hidden]')
          .then(this.next);
    },
    function() {
      checkIfNoErrorsOccured(this.next);
    },
  ]);
};
