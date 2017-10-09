// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ExtensionsAccessibilityTest fixture.
GEN_INCLUDE([
  'extensions_accessibility_test.js',
]);

AccessibilityTest.define('ExtensionsAccessibilityTest', {
  /** @override */
  name: 'BASIC_EXTENSIONS',

  /** @override */
  axeOptions: ExtensionsAccessibilityTest.axeOptions,

  /** @override */
  tests: {'Accessible with No Changes': function() {}},
});
