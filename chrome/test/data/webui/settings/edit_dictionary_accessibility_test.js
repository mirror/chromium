// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * @fileoverview Suite of accessibility tests for the EDIT_DICTIONARY route.
 */

suite('EDIT_DICTIONARY', function() {
  setup(function() {
    // Reset the blank to be a blank page.
    PolymerTest.clearBody();

    // Set the URL of the page to render to load the correct view upon
    // injecting settings-ui without attaching listeners.
    window.history.pushState('object or string', 'Test', '/editDictionary');

    var settingsUi = document.createElement('settings-ui');
    document.body.appendChild(settingsUi);
    Polymer.dom.flush();
  });

  // TODO(quacht): enable the 'aria-valid-attr-rule' once axe-core supports
  // exclusion of the input element, which raises a false positive.
  test('Accessible with No Changes', function() {
    var auditOptions = {
      context: {exclude: ['iron-iconset-svg']},
      options: {
        'rules': {
          'aria-valid-attr-value': {enabled: false},
        }
      }
    };
    return SettingsAccessibilityTest.runAudit(auditOptions);
  });
});
