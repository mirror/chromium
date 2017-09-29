// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/tab_under_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "chrome/browser/ui/blocked_content/popup_opener_tab_helper.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "url/gurl.h"
#include "url/origin.h"

const base::Feature TabUnderNavigationThrottle::kBlockTabUnders{
    "BlockTabUnders", base::FEATURE_DISABLED_BY_DEFAULT};

// Variation parameter used to determine when the duration of time between popup
// and suspicious client redirect makes the client redirect considered a
// tab-under. If this is 0 or unset then the threshold is infinite.
const char kTabUnderPopupThresholdParam[] = "tab_under_popup_threshold_ms";
const int kTabUnderPopupThresholdDefault = 0;

// static
std::unique_ptr<content::NavigationThrottle>
TabUnderNavigationThrottle::MaybeCreate(content::NavigationHandle* handle) {
  if (handle->IsInMainFrame() && base::FeatureList::IsEnabled(kBlockTabUnders))
    return base::WrapUnique(new TabUnderNavigationThrottle(handle));
  return nullptr;
}

// static
bool TabUnderNavigationThrottle::IsSuspiciousClientRedirect(
    content::NavigationHandle* navigation_handle,
    bool started_in_background) {
  // Only care about renderer initiated navigations without user gestures.
  if (!started_in_background || !navigation_handle->IsInMainFrame() ||
      navigation_handle->HasUserGesture() ||
      !navigation_handle->IsRendererInitiated()) {
    return false;
  }

  // An empty previous URL indicates this was the first load. We filter these
  // out because we're primarily interested in sites which navigate themselves
  // away while in the background.
  const GURL& previous_main_frame_url =
      navigation_handle->HasCommitted()
          ? navigation_handle->GetPreviousURL()
          : navigation_handle->GetWebContents()->GetLastCommittedURL();
  if (previous_main_frame_url.is_empty())
    return false;

  // Only cross origin navigations are considered tab-unders.
  if (url::Origin(previous_main_frame_url)
          .IsSameOriginWith(url::Origin(navigation_handle->GetURL()))) {
    return false;
  }

  return true;
}

TabUnderNavigationThrottle::~TabUnderNavigationThrottle() = default;

TabUnderNavigationThrottle::TabUnderNavigationThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle),
      tab_under_popup_threshold_(base::TimeDelta::FromMilliseconds(
          base::GetFieldTrialParamByFeatureAsInt(
              kBlockTabUnders,
              kTabUnderPopupThresholdParam,
              kTabUnderPopupThresholdDefault))) {}

bool TabUnderNavigationThrottle::IsTimeSinceLastPopupConsideredTabUnder(
    const base::Optional<base::TimeDelta>& time_since_last_popup) const {
  if (!time_since_last_popup.has_value())
    return false;
  return tab_under_popup_threshold_.is_zero() ||
         time_since_last_popup.value() <= tab_under_popup_threshold_;
}

bool TabUnderNavigationThrottle::ShouldBlockNavigation() const {
  auto* popup_opener = PopupOpenerTabHelper::FromWebContents(
      navigation_handle()->GetWebContents());
  return popup_opener &&
         IsTimeSinceLastPopupConsideredTabUnder(
             popup_opener->TimeSinceLastPopup()) &&
         IsSuspiciousClientRedirect(navigation_handle(),
                                    started_in_background_);
}

content::NavigationThrottle::ThrottleCheckResult
TabUnderNavigationThrottle::MaybeBlockNavigation() {
  content::WebContents* contents = navigation_handle()->GetWebContents();
  content::WebContentsDelegate* delegate = contents->GetDelegate();
  if (delegate && ShouldBlockNavigation()) {
    delegate->OnDidBlockFramebust(contents, navigation_handle()->GetURL());
    return content::NavigationThrottle::CANCEL;
  }
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
TabUnderNavigationThrottle::WillStartRequest() {
  started_in_background_ = !navigation_handle()->GetWebContents()->IsVisible();
  return MaybeBlockNavigation();
}

content::NavigationThrottle::ThrottleCheckResult
TabUnderNavigationThrottle::WillRedirectRequest() {
  return MaybeBlockNavigation();
}

const char* TabUnderNavigationThrottle::GetNameForLogging() {
  return "TabUnderNavigationThrottle";
}
