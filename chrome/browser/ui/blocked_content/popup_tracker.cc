// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popup_tracker.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/ui/blocked_content/popup_opener_tab_helper.h"
#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

#define UMA_HISTOGRAM_LONG_TIMES_6H(name, sample)                  \
  UMA_HISTOGRAM_CUSTOM_TIMES(name, sample,                         \
                             base::TimeDelta::FromMilliseconds(1), \
                             base::TimeDelta::FromHours(6), 50)

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PopupTracker);

void PopupTracker::CreateForWebContents(content::WebContents* contents,
                                        content::WebContents* opener,
                                        bool untrusted) {
  DCHECK(contents);
  DCHECK(opener);
  if (!FromWebContents(contents)) {
    contents->SetUserData(UserDataKey(), base::WrapUnique(new PopupTracker(
                                             contents, opener, untrusted)));
  }
}

PopupTracker::~PopupTracker() {
  base::Optional<base::TimeDelta> visible_time_on_first_document =
      GetVisibleTimeOnFirstDocument();
  if (visible_time_on_first_document.has_value()) {
    UMA_HISTOGRAM_LONG_TIMES(
        "ContentSettings.Popups.FirstDocumentEngagementTime2",
        visible_time_on_first_document.value());
  }
  base::TimeDelta total_visible_time =
      visibility_tracker_->GetForegroundDuration();
  UMA_HISTOGRAM_LONG_TIMES_6H("Tab.VisibleTime.Popup", total_visible_time);
  if (is_untrusted_) {
    UMA_HISTOGRAM_LONG_TIMES_6H("Tab.VisibleTime.Popup.Blockable",
                                total_visible_time);
  }
}

PopupTracker::PopupTracker(content::WebContents* contents,
                           content::WebContents* opener,
                           bool untrusted)
    : content::WebContentsObserver(contents),
      tick_clock_(std::make_unique<base::DefaultTickClock>()),
      is_untrusted_(untrusted) {
  visibility_tracker_ = std::make_unique<ScopedVisibilityTracker>(
      tick_clock_.get(), web_contents()->IsVisible());
  if (auto* popup_opener = PopupOpenerTabHelper::FromWebContents(opener))
    popup_opener->OnOpenedPopup(this);
}

void PopupTracker::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (!visible_time_before_first_document_.has_value()) {
    visible_time_before_first_document_ =
        visibility_tracker_->GetForegroundDuration();
  } else if (!visible_time_on_first_document_.has_value()) {
    visible_time_on_first_document_ = GetVisibleTimeOnFirstDocument();
  }
}

void PopupTracker::WasShown() {
  visibility_tracker_->OnShown();
}

void PopupTracker::WasHidden() {
  visibility_tracker_->OnHidden();
}

base::Optional<base::TimeDelta> PopupTracker::GetVisibleTimeOnFirstDocument()
    const {
  if (!visible_time_before_first_document_.has_value())
    return base::Optional<base::TimeDelta>();

  if (visible_time_on_first_document_.has_value())
    return visible_time_on_first_document_.value();

  return visibility_tracker_->GetForegroundDuration() -
         visible_time_before_first_document_.value();
}
