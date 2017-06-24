// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the Media Engagement WebUI.
 */
var ROOT_PATH = '../../../../../';
var EXAMPLE_URL_1 = 'http://example.com/';
var EXAMPLE_URL_2 = 'http://shmlexample.com/';

GEN('#include "base/command_line.h"');
GEN('#include "chrome/browser/media/media_engagement_service.h"');
GEN('#include "chrome/browser/media/media_engagement_service_factory.h"');
GEN('#include "chrome/browser/ui/browser.h"');

function MediaEngagementBrowserTest() {}

MediaEngagementBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  browsePreload: 'chrome://media-engagement',

  commandLineSwitches: [{
    switchName: 'enable-features',
    switchValue: 'media-engagement'
  }],

  runAccessibilityChecks: false,

  isAsync: true,

  testGenPreamble: function() {
    GEN('MediaEngagementService* service =');
    GEN('  MediaEngagementServiceFactory::GetForProfile(');
    GEN('    browser()->profile());');
    GEN('service->HandleInteraction(GURL("' + EXAMPLE_URL_1 +
        '"), MediaEngagementService::InteractionTypes::INTERACTION_VISIT);');
    GEN('service->HandleInteraction(GURL("' + EXAMPLE_URL_2 +
        '"), MediaEngagementService::InteractionTypes::INTERACTION_VISIT);');
  },

  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],

  /** @override */
  setUp: function() {
    testing.Test.prototype.setUp.call(this);
    suiteSetup(function() {
      return whenPageIsPopulatedForTest();
    });
  },
};

TEST_F('MediaEngagementBrowserTest', 'All', function() {
  test('check engagement values are loaded', function() {
    var originCells =
        Array.from(document.getElementsByClassName('origin-cell'));
    assertDeepEquals(
        [EXAMPLE_URL_1, EXAMPLE_URL_2], originCells.map(x => x.textContent));
  });

  mocha.run();
});
