// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/redirector_visibility_tracker.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/origin.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(RedirectorVisibilityTracker);

RedirectorVisibilityTracker::~RedirectorVisibilityTracker() {
  if (visibility_tracker_) {
    UMA_HISTOGRAM_LONG_TIMES("Tab.VisibleTimeAfterCrossOriginRedirect",
                             visibility_tracker_->GetForegroundDuration());
  }
}

RedirectorVisibilityTracker::RedirectorVisibilityTracker(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void RedirectorVisibilityTracker::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() || navigation_handle->IsErrorPage()) {
    return;
  }

  // This tab has already redirected cross site.
  if (visibility_tracker_)
    return;

  // Only consider navigations which finish in the background.
  if (web_contents()->IsVisible())
    return;

  // An empty previous URL indicates this was the first load.
  const GURL& previous_main_frame_url = navigation_handle->GetPreviousURL();
  if (previous_main_frame_url.is_empty())
    return;

  if (url::Origin(previous_main_frame_url)
          .IsSameOriginWith(url::Origin(navigation_handle->GetURL()))) {
    return;
  }

  visibility_tracker_ = base::MakeUnique<ScopedVisibilityTracker>(
      base::MakeUnique<base::DefaultTickClock>(), false /* is_visible */);
}

void RedirectorVisibilityTracker::WasShown() {
  if (visibility_tracker_)
    visibility_tracker_->OnShown();
}

void RedirectorVisibilityTracker::WasHidden() {
  if (visibility_tracker_)
    visibility_tracker_->OnHidden();
}
