// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-options-dialog. */
cr.define('extension_options_dialog_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Layout: 'Layout',
  };

  var suiteName = 'ExtensionOptionsDialogTests';

  suite(suiteName, function() {
    /** @type {extensions.OptionsDialog} */
    var optionsDialog;

    /** @type {chrome.developerPrivate.ExtensionInfo} */
    var data;

    /** @type {string} */
    var extensionId = 'pmhmnldgoomonepbmfhoeieppdegdicf';

    setup(function() {
      console.log('----------setup running');
      PolymerTest.clearBody();
      data = extension_test_util.createExtensionInfo({id: extensionId});
      optionsDialog = new extensions.OptionsDialog();
      document.body.appendChild(optionsDialog);
    });

    function isDialogVisible() {
      var dialogElement = optionsDialog.$$('dialog');
      var rect = dialogElement.getBoundingClientRect();
      return rect.width * rect.height > 0;
    }

    test(assert(TestNames.Layout), function() {
      console.log('----------got here 0');
      // Try showing the dialog.
      assertFalse(isDialogVisible());
      optionsDialog.show(data);
      console.log('----------got here 1');
      console.log('-----------' + test_util);
      console.log('-----------' + test_util.whenAttributeIs);
      return test_util.whenAttributeIs(
          optionsDialog.$.dialog, 'open', '').then(function() {
        console.log('----------got here 2');
        assertTrue(isDialogVisible());

        assertEquals(
            data.name,
            assert(optionsDialog.$$('#icon-and-name-wrapper span'))
                .textContent.trim());

        var optionEle = optionsDialog.$$('extensionoptions');

        var mockOptions = optionsDialog.extensionOptions_;
        assertEquals(data.id, mockOptions.extension);

        // Setting the preferred width to something below the min width
        // changes the width property. But visually, min-width still prevails.
        mockOptions.onpreferredsizechanged({height: 100, width: 100});
        assertEquals('100px', optionEle.style.width);
        var computedStyle = window.getComputedStyle(optionEle);
        assertEquals('300px', computedStyle.minWidth);

        // Setting the preferred size to between the min and max dimensions
        // should change the dimensions.
        mockOptions.onpreferredsizechanged({height: 500, width: 400});
        assertEquals('500px', optionEle.style.height);
        assertEquals('400px', optionEle.style.width);

        mockOptions.onclose();
        assertFalse(isDialogVisible());
      });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
