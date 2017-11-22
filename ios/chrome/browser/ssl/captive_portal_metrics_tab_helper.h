// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_METRICS_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_METRICS_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "ios/chrome/browser/ssl/captive_portal_metrics.h"
#import "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

namespace web {
class WebState;
}

// Logs the result of a captive portal detector when users enter error states
// which could be caused by being on a Captive Portal network.
// 1. When a -1200 error occurs.
// 2. If a request is taking a long time to complete.
class CaptivePortalMetricsTabHelper
    : public web::WebStateObserver,
      public web::WebStateUserData<CaptivePortalMetricsTabHelper> {
 public:
  ~CaptivePortalMetricsTabHelper() override;

 private:
  explicit CaptivePortalMetricsTabHelper(web::WebState* web_state);
  friend class web::WebStateUserData<CaptivePortalMetricsTabHelper>;

  // Checks the current captive portal state and calls |callback| with the
  // result.
  void TestForCaptivePortal(
      const captive_portal::CaptivePortalDetector::DetectionCallback& callback);
  // Cancels the timer used to check for a blocking captive portal.
  void CancelTimer();
  // Performs a test for a captive portal after |timer_| has triggered.
  void TimerTriggered();
  // Returns the CaptivePortalStatus corresponding to |results.result|.
  static CaptivePortalStatus CaptivePortalStatusFromDetectionResult(
      const captive_portal::CaptivePortalDetector::Results& results);

  // web::WebStateObserver implementation.
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void DidStartLoading(web::WebState* web_state) override;
  void DidStopLoading(web::WebState* web_state) override;
  void WebStateDestroyed(web::WebState* web_state) override;

  // A timer to test for a captive portal blocking an ongoing request.
  base::OneShotTimer timer_;
  // The web state associated with this tab helper.
  web::WebState* web_state_;

  base::WeakPtrFactory<CaptivePortalMetricsTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalMetricsTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_METRICS_TAB_HELPER_H_
