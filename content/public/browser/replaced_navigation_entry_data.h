// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_REPLACED_NAVIGATION_ENTRY_DATA_H_
#define CONTENT_PUBLIC_BROWSER_REPLACED_NAVIGATION_ENTRY_DATA_H_

#include "base/time/time.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace content {

// Represents a subset of NavigationEntry's fields, stored in this structure for
// the cases where a navigation has been replaced (e.g. history.replaceState())
// and the original values must be remembered. The main goal is to make sure
// important user-initiated navigations like PAGE_TRANSITION_TYPED and
// PAGE_TRANSITION_AUTO_BOOKMARK are not "lost" due to later replacements.
//
// Things worth pointing out:
// - This is the first commit that happens for a given NavigationEntry, before
//   any client redirects, location.replace() events, or history.replaceState()
//   events.
//
// - This might not the same as the first entry in the redirect chain or the
//   original_request_url, since it represents the landing URL in the original
//   server-side redirect chain.
//
// - The value is preserved for navigations modifying history, including:
//   a) Cross-document replacement navigations which generate a new
//      NavigationEntry with replacement (e.g. client redirects and
//      location.replace())
//   b) Same-document cases that update the existing NavigationEntry (e.g.,
//      history.replaceState()).
//
// - Keeping a single value of this struct (per navigation controller index) is
//   sufficient because the relevant page transition that we care about
//   (PAGE_TRANSITION_TYPED and PAGE_TRANSITION_AUTOBOOKMARK) always create a
//   new entry.
//
// TODO/DONOTSUBMIT: Clarify something about only tracking this for main frames,
// even though the concept is valid for subframes?
struct ReplacedNavigationEntryData {
  // Analogous to NavigationEntry::GetURL(). Note that, in the event of server-
  // side redirects, it stores the landing URL.
  GURL url;
  // Analogous to NavigationEntry::GetTimestamp(). This is useful during
  // analysis of sync-ed history to distinguish whether the entry prior to being
  // replaced was sync-ed or not.
  base::Time timestamp;
  // Analogous to NavigationEntry::GetTransitionType().
  ui::PageTransition transition_type;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_REPLACED_NAVIGATION_ENTRY_DATA_H_
