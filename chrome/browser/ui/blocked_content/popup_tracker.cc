// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popup_tracker.h"

#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/blocked_content/popup_blocker_tab_helper.h"
#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(PopupTracker);

// static
void PopupTracker::CreateForWebContents(content::WebContents* web_contents,
                                        content::WebContents* opener) {
  if (!FromWebContents(web_contents)) {
    web_contents->SetUserData(UserDataKey(), base::WrapUnique(new PopupTracker(
                                                 web_contents, opener)));
  }
}

PopupTracker::~PopupTracker() {
  if (first_load_visibility_tracker_) {
    UMA_HISTOGRAM_LONG_TIMES(
        "ContentSettings.Popups.FirstDocumentEngagementTime",
        first_load_visibility_tracker_->GetForegroundDuration());
  }
  if (opener_)
    opener_->RemovePopup(this);
}

PopupTracker::PopupTracker(content::WebContents* web_contents,
                           content::WebContents* opener)
    : content::WebContentsObserver(web_contents) {
  PopupOpenerTracker::CreateForWebContents(opener);
  opener_ = PopupOpenerTracker::FromWebContents(opener);
  opener_->AddPopup(this);
}

void PopupTracker::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // The existence of |first_load_visibility_tracker_| is a proxy for whether
  // we've committed the first navigation in this WebContents.
  if (!first_load_visibility_tracker_) {
    first_load_visibility_tracker_ = base::MakeUnique<ScopedVisibilityTracker>(
        base::MakeUnique<base::DefaultTickClock>(),
        web_contents()->IsVisible());
    if (!first_navigation_closure_.is_null())
      std::move(first_navigation_closure_).Run();
  } else {
    web_contents()->RemoveUserData(UserDataKey());
    // Deletes this.
  }
}

void PopupTracker::WasShown() {
  if (first_load_visibility_tracker_)
    first_load_visibility_tracker_->OnShown();
}

void PopupTracker::WasHidden() {
  if (first_load_visibility_tracker_)
    first_load_visibility_tracker_->OnHidden();
}

bool PopupTracker::OnDidFinishNavigationInOpener(
    content::NavigationHandle* handle) {
  DCHECK(opener_);
  bool should_close = !handle->HasUserGesture();
  if (should_close) {
    chrome::NavigateParams params(
        Profile::FromBrowserContext(web_contents()->GetBrowserContext()),
        handle->GetURL(), handle->GetPageTransition());
    blink::mojom::WindowFeatures window_features;
    DCHECK(first_navigation_closure_.is_null());
    if (first_load_visibility_tracker_) {
      MaybeCloseOpener(params, window_features);
    } else {
      first_navigation_closure_ =
          base::BindOnce(&PopupTracker::MaybeCloseOpener,
                         base::Unretained(this), params, window_features);
    }
  }
  return should_close;
}

void PopupTracker::OnOpenerGoingAway() {
  opener_ = nullptr;
}

void PopupTracker::MaybeCloseOpener(
    const chrome::NavigateParams& params,
    const blink::mojom::WindowFeatures& window_features) {
  // The opener could have closed itself by now.
  if (!opener_)
    return;

  // Check for content setting exception for this URL.
  const GURL& popup_url = web_contents()->GetLastCommittedURL();
  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  if (popup_url.is_valid() &&
      HostContentSettingsMapFactory::GetForProfile(profile)->GetContentSetting(
          popup_url, popup_url, CONTENT_SETTINGS_TYPE_POPUPS, std::string()) ==
          CONTENT_SETTING_ALLOW) {
    return;
  }

  // No exception, "block" the original opener by closing it.
  // TODO(csharrison): Log to console an error/warning.
  opener_->CloseAsync();
  PopupBlockerTabHelper::FromWebContents(web_contents())
      ->AddBlockedPopup(params, window_features);
}
