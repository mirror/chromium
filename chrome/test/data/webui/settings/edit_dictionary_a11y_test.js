// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the EDIT_DICTIONARY route.
 */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// SettingsAccessibilityTest fixture.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/settings/accessibility_browsertest.js',
]);

AccessibilityTest.define('SettingsAccessibilityTest', {
  /** @override */
  name: 'EDIT_DICTIONARY',
  /** @override */
  setup: function() {
    settings.navigateTo(settings.routes.EDIT_DICTIONARY);
    Polymer.dom.flush();
  },
  /** @override */
  tests: {
    'Accessible with No Changes': function() {}
  },
  /** @override */
  violationFilter: {
    'aria-valid-attr': function(nodeResult) {
      return nodeResult.element.hasAttribute('aria-active-attribute');
    },
    'aria-valid-attr-value': function(nodeResult) {
      var describerId = nodeResult.element.getAttribute('aria-describedby');
      return  describerId == 'cloudPrintersSecondary' ||
          (describerId == '' && nodeResult.element.id == 'input');
    },
    'button-name': function(nodeResult) {
      var node = nodeResult.element;
      return node.className = 'icon-expand-more';
    },
  },
});