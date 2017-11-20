// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const {string} Path to source root. */
const ROOT_PATH = '../../../../../';

/**
 * TestFixture for Discards WebUI testing.
 * @extends {testing.Test}
 * @constructor
 */
function DiscardsTest() {}
DiscardsTest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to the options page and call preLoad().
   * @override
   */
  browsePreload: 'chrome://discards',
};

TEST_F('DiscardsTest', 'TestCompareTabDiscardsInfo', function() {
  let dummy1 = {
    title: 'title 1',
    tabUrl: 'http://urlone.com',
    isApp: false,
    isInternal: false,
    isMedia: false,
    isPinned: false,
    isDiscarded: false,
    isAutoDiscardable: false,
    discardCount: 0,
    utilityRank: 0,
    lastActiveSeconds: 0
  };
  let dummy2 = {
    title: 'title 2',
    tabUrl: 'http://urltwo.com',
    isApp: true,
    isInternal: true,
    isMedia: true,
    isPinned: true,
    isDiscarded: true,
    isAutoDiscardable: true,
    discardCount: 1,
    utilityRank: 1,
    lastActiveSeconds: 1
  };

  ['title', 'tabUrl', 'isApp', 'isInternal', 'isMedia', 'isPinned',
      'isDiscarded', 'isAutoDiscardable', 'discardCount', 'utilityRank',
      'lastActiveSeconds'].forEach((sortKey) => {
    assertTrue(
        discards.compareTabDiscardsInfos(sortKey, dummy1, dummy2) < 0);
    assertTrue(
        discards.compareTabDiscardsInfos(sortKey, dummy2, dummy1) > 0);
    assertTrue(
        discards.compareTabDiscardsInfos(sortKey, dummy1, dummy1) == 0);
    assertTrue(
        discards.compareTabDiscardsInfos(sortKey, dummy2, dummy2) == 0);
  });
});

TEST_F('DiscardsTest', 'TestLastActiveToString', function() {
  assertEquals('just now', discards.lastActiveToString(0));
  assertEquals('just now', discards.lastActiveToString(10));
  assertEquals('just now', discards.lastActiveToString(59));
  assertEquals('1 minute ago', discards.lastActiveToString(60));
  assertEquals('10 minutes ago', discards.lastActiveToString(
      10 * 60 + 30));
  assertEquals('59 minutes ago', discards.lastActiveToString(
      59 * 60 + 59));
  assertEquals('1 hour ago', discards.lastActiveToString(
      60 * 60));
  assertEquals('1 hour and 1 minute ago', discards.lastActiveToString(
      61 * 60));
  assertEquals('1 hour and 10 minutes ago', discards.lastActiveToString(
      70 * 60 + 30));
  assertEquals('1 day ago', discards.lastActiveToString(
      24 * 60 * 60));
  assertEquals('2 days ago', discards.lastActiveToString(
      2.5 * 24 * 60 * 60));
  assertEquals('6 days ago', discards.lastActiveToString(
      6.9 * 24 * 60 * 60));
  assertEquals('over 1 week ago', discards.lastActiveToString(
      7 * 24 * 60 * 60));
  assertEquals('over 2 weeks ago', discards.lastActiveToString(
      2.5 * 7 * 24 * 60 * 60));
  assertEquals('over 4 weeks ago', discards.lastActiveToString(
      30 * 24 * 60 * 60));
  assertEquals('over 1 month ago', discards.lastActiveToString(
      30.5 * 24 * 60 * 60));
  assertEquals('over 2 months ago', discards.lastActiveToString(
      2.5 * 30.5 * 24 * 60 * 60));
  assertEquals('over 11 months ago', discards.lastActiveToString(
      364 * 24 * 60 * 60));
  assertEquals('over 1 year ago', discards.lastActiveToString(
      365 * 24 * 60 * 60));
  assertEquals('over 2 years ago', discards.lastActiveToString(
      2.3 * 365 * 24 * 60 * 60));
});

TEST_F('DiscardsTest', 'TestMaybeMakePlural', function() {
  assertEquals('hours', discards.maybeMakePlural('hour', 0));
  assertEquals('hour', discards.maybeMakePlural('hour', 1));
  assertEquals('hours', discards.maybeMakePlural('hour', 2));
  assertEquals('hours', discards.maybeMakePlural('hour', 10));
});
