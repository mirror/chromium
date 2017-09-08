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

// static
void PopupTracker::CreateForWebContents(content::WebContents* web_contents,
                                        Type type) {
  DCHECK(web_contents);
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(
        UserDataKey(), base::WrapUnique(new PopupTracker(web_contents, type)));
  }
}

// static
PopupTracker::Type PopupTracker::PopupTrackerTypeForTriggeringEvent(
    blink::WebTriggeringEventInfo triggering_event_info) {
  switch (triggering_event_info) {
    case blink::WebTriggeringEventInfo::kUnknown:
      return Type::kOpenUrlTrustedEventUnknown;
    case blink::WebTriggeringEventInfo::kNotFromEvent:
      return Type::kOpenUrlNotFromEvent;
    case blink::WebTriggeringEventInfo::kFromTrustedEvent:
      return Type::kOpenUrlTrustedEvent;
    case blink::WebTriggeringEventInfo::kFromUntrustedEvent:
      return Type::kOpenUrlUntrustedEvent;
    case blink::WebTriggeringEventInfo::kLast:
      NOTREACHED();
      return Type::kLast;
  }
}

PopupTracker::~PopupTracker() {
  if (visibility_tracker_) {
    UMA_HISTOGRAM_LONG_TIMES("Popups.FirstDocument.EngagementTime",
                             visibility_tracker_->GetForegroundDuration());
  }
  UMA_HISTOGRAM_ENUMERATION("Popups.Type", type_, Type::kLast);
}

PopupTracker::PopupTracker(content::WebContents* web_contents, Type type)
    : content::WebContentsObserver(web_contents), type_(type) {}

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
