// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function verifyTextElementTimingEntries(expectedNames) {
  var bufferedEntries = performance.getEntriesByType("elementtiming");
  assert_equals(bufferedEntries.length, expectedNames.length, "Wrong number of elementtiming entries");
  bufferedEntries.sort(function(a, b) { return a.name < b.name ? -1 : (a.name > b.name ? 1 : 0); });

  for (var i = 0; i < expectedNames.length; i++) {
    assert_equals(bufferedEntries[i].entryType, "elementtiming");
    assert_equals(bufferedEntries[i].elementType, "text");
    assert_equals(bufferedEntries[i].name, expectedNames[i]);
    assert_equals(bufferedEntries[i].duration, 0);
    assert_greater_than(bufferedEntries[i].startTime, 0);
  }
}
