// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extension-keyboard-shortcuts. */
cr.define('extension_kiosk_mode_tests', function() {
  /** @enum {string} */
  let TestNames = {
    ButtonModal: 'ButtonModal',
    AddButton: 'AddButton',
  };

  let SuiteNames = {
    Basic: 'BasicSuite',
    DialogOnly: 'DialogOnlySuite',
  };

  suite(assert(SuiteNames.Basic), function() {
    test(assert(TestNames.ButtonModal), function() {
      const button = document.querySelector('* /deep/ #kiosk-extensions');
      expectTrue(!!button);

      MockInteractions.tap(button);
      Polymer.dom.flush();
      const dialog = document.querySelector('* /deep/ #kiosk-dialog');
      expectTrue(!!dialog);
    });
  });

  suite(assert(SuiteNames.DialogOnly), function() {
    let dialog;

    setup(function() {
      PolymerTest.clearBody();

      dialog = document.createElement('extensions-kiosk-dialog');
      document.body.appendChild(dialog);

      Polymer.dom.flush();
    });

    test(assert(TestNames.AddButton), function() {
      const addButton = dialog.$['add-button'];
      expectTrue(!!addButton);
      expectTrue(addButton.disabled);

      const addInput = dialog.$['add-input'];
      addInput.value = 'blah';
      expectFalse(addButton.disabled);
    });
  });

  return {
    SuiteNames: SuiteNames,
    TestNames: TestNames,
  };
});
