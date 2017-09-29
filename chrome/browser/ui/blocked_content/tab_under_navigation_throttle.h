// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_TAB_UNDER_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_TAB_UNDER_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}

// This class blocks navigations that we've classified as tab-unders. It does so
// by communicating with the popup opener tab helper.
class TabUnderNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreate(
      content::NavigationHandle* handle);

  // Suspicious redirects are main frame navigations which:
  // 1. Have no user gesture
  // 2. Start in the background
  // 3. Are cross origin
  // 4. Are not the first navigation in the WebContents
  //
  // Note: Pass in |started_in_background| because depending on the state the
  // navigation is in, we need additional data to determine whether it started
  // in the background.
  //
  // This method should be robust to navigations at any stage.
  static bool IsSuspiciousClientRedirect(
      content::NavigationHandle* navigation_handle,
      bool started_in_background);

  ~TabUnderNavigationThrottle() override;

 private:
  explicit TabUnderNavigationThrottle(content::NavigationHandle* handle);

  bool IsTimeSinceLastPopupConsideredTabUnder(
      const base::Optional<base::TimeDelta>& time_since_last_popup) const;
  bool ShouldBlockNavigation() const;
  content::NavigationThrottle::ThrottleCheckResult MaybeBlockNavigation();

  // content::NavigationThrottle:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

  // The threshold from when a suspicious redirect happens after a popup to be
  // considered a tab-under.
  base::TimeDelta tab_under_popup_threshold_;

  DISALLOW_COPY_AND_ASSIGN(TabUnderNavigationThrottle);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_TAB_UNDER_NAVIGATION_THROTTLE_H_
