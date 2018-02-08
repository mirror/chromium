// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_stats.h"
#include "chrome/browser/search/search.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "components/bookmarks/browser/bookmark_model.h"

using bookmarks::BookmarkNode;

void RecordBookmarkLaunch(const content::WebContents* contents,
                          const BookmarkNode* node,
                          BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(base::UserMetricsAction("ClickedBookmarkBarURLButton"));
  }
  UMA_HISTOGRAM_ENUMERATION(
      "Bookmarks.LaunchLocation", location, BOOKMARK_LAUNCH_LOCATION_LIMIT);

  // If New Tab Page state cannot be determined, nothing is recorded here.
  if (contents) {
    if (search::IsInstantNTP(contents)) {
      UMA_HISTOGRAM_ENUMERATION("Bookmarks.LaunchLocation",
                                BOOKMARK_LAUNCH_LOCATION_NTP,
                                BOOKMARK_LAUNCH_LOCATION_LIMIT);
    } else {
      UMA_HISTOGRAM_ENUMERATION("Bookmarks.LaunchLocation",
                                BOOKMARK_LAUNCH_LOCATION_NOT_NTP,
                                BOOKMARK_LAUNCH_LOCATION_LIMIT);
    }
  }
  if (!node)
    return;

  // In the cases where a bookmark node is provided, record the depth of the
  // bookmark in the tree.
  int depth = 0;
  for (const BookmarkNode* iter = node; iter != NULL; iter = iter->parent()) {
    depth++;
  }
  // Record |depth - 2| to offset the invisible root node and permanent nodes
  // (Bookmark Bar, Mobile Bookmarks or Other Bookmarks)
  UMA_HISTOGRAM_COUNTS("Bookmarks.LaunchDepth", depth - 2);
}

void RecordBookmarkFolderOpen(BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(base::UserMetricsAction("ClickedBookmarkBarFolder"));
  }
}

void RecordBookmarkAppsPageOpen(BookmarkLaunchLocation location) {
  if (location == BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR ||
      location == BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR) {
    base::RecordAction(
        base::UserMetricsAction("ClickedBookmarkBarAppsShortcutButton"));
  }
}
