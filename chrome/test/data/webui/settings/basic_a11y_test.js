// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the BASIC route.
 */

AccessibilityTest.define({
  /** @override */
  name: 'BASIC',

  /** @override */
  tests: {
    'Accessible with No Changes': function() {}
  },
  violationFilter: {
    // TODO(quacht): remove this exception once the color contrast issue is
    // solved.
    // http://crbug.com/748608
    'color-contrast': function(nodeResult) {
      return nodeResult.element.id == 'prompt';
    },
   'aria-valid-attr': function(nodeResult) {
      return nodeResult.element.hasAttribute('aria-active-attribute');
    },
  }
});
