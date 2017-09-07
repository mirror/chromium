// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popup_tracker.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PopupTracker);

PopupTracker::~PopupTracker() {
  if (visibility_tracker_) {
    base::TimeDelta foreground_time =
        visibility_tracker_->GetForegroundDuration();
    UMA_HISTOGRAM_LONG_TIMES("Popups.FirstDocument.EngagementTime",
                             foreground_time);
  }
}

PopupTracker::PopupTracker(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}

void PopupTracker::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (!visibility_tracker_) {
    visibility_tracker_ = base::MakeUnique<ScopedVisibilityTracker>(
        base::MakeUnique<base::DefaultTickClock>(),
        web_contents()->IsVisible());
  } else {
    web_contents()->RemoveUserData(UserDataKey());
    // Destroys this object.
  }
}

void PopupTracker::WasShown() {
  if (visibility_tracker_)
    visibility_tracker_->OnShown();
}

void PopupTracker::WasHidden() {
  if (visibility_tracker_)
    visibility_tracker_->OnHidden();
}
