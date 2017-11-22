// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/captive_portal_metrics_tab_helper.h"

#import <CFNetwork/CFNetwork.h>

#include "base/mac/bind_objc_block.h"
#include "base/metrics/histogram_macros.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"
#import "ios/web/public/web_state/navigation_context.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(CaptivePortalMetricsTabHelper);

const char kCaptivePortalCausingTimeoutHistogram[] =
    "CaptivePortal.Session.TimeoutDetectionResult";

const char kCaptivePortalSecureConnectionFailedHistogram[] =
    "CaptivePortal.Session.SecureConnectionFailed";

// Time in milliseconds of still ongoing request before testing if the user is
// behind a captive portal.
const int64_t kCaptivePortalTestDelayInMilliseconds = 8000;

CaptivePortalMetricsTabHelper::CaptivePortalMetricsTabHelper(
    web::WebState* web_state)
    : web_state_(web_state), weak_factory_(this) {
  web_state_->AddObserver(this);
}

CaptivePortalMetricsTabHelper::~CaptivePortalMetricsTabHelper() {}

void CaptivePortalMetricsTabHelper::TestForCaptivePortal(
    const captive_portal::CaptivePortalDetector::DetectionCallback& callback) {
  CaptivePortalDetectorTabHelper* tab_helper =
      CaptivePortalDetectorTabHelper::FromWebState(web_state_);
  // TODO(crbug.com/760873): replace test with DCHECK when this method is only
  // called on WebStates attached to tabs.
  if (tab_helper) {
    tab_helper->detector()->DetectCaptivePortal(
        GURL(captive_portal::CaptivePortalDetector::kDefaultURL), callback,
        NO_TRAFFIC_ANNOTATION_YET);
  }
}

void CaptivePortalMetricsTabHelper::CancelTimer() {
  timer_.Stop();
}

void CaptivePortalMetricsTabHelper::TimerTriggered() {
  TestForCaptivePortal(base::BindBlockArc(^(
      const captive_portal::CaptivePortalDetector::Results& results) {
    CaptivePortalStatus status =
        CaptivePortalMetricsTabHelper::CaptivePortalStatusFromDetectionResult(
            results);
    UMA_HISTOGRAM_ENUMERATION(kCaptivePortalCausingTimeoutHistogram,
                              static_cast<int>(status),
                              static_cast<int>(CaptivePortalStatus::COUNT));
  }));
}

// static
CaptivePortalStatus
CaptivePortalMetricsTabHelper::CaptivePortalStatusFromDetectionResult(
    const captive_portal::CaptivePortalDetector::Results& results) {
  CaptivePortalStatus status;
  switch (results.result) {
    case captive_portal::RESULT_INTERNET_CONNECTED:
      status = CaptivePortalStatus::ONLINE;
      break;
    case captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL:
      status = CaptivePortalStatus::PORTAL;
      break;
    default:
      status = CaptivePortalStatus::UNKNOWN;
      break;
  }
  return status;
}

// WebStateObserver
void CaptivePortalMetricsTabHelper::DidStartLoading(web::WebState* web_state) {
  CancelTimer();

  timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMilliseconds(kCaptivePortalTestDelayInMilliseconds),
      this, &CaptivePortalMetricsTabHelper::TimerTriggered);
}

void CaptivePortalMetricsTabHelper::DidStopLoading(web::WebState* web_state) {
  CancelTimer();
}

void CaptivePortalMetricsTabHelper::WebStateDestroyed(
    web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);

  CancelTimer();

  // Ensure the captive portal detection is canceled if it never completed.
  CaptivePortalDetectorTabHelper* tab_helper =
      CaptivePortalDetectorTabHelper::FromWebState(web_state_);
  // TODO(crbug.com/760873): replace test with DCHECK when this method is only
  // called on WebStates attached to tabs.
  if (tab_helper) {
    tab_helper->detector()->Cancel();
  }

  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

void CaptivePortalMetricsTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  NSError* error = navigation_context->GetError();
  if ([NSURLErrorDomain isEqualToString:error.domain] &&
      error.code == kCFURLErrorSecureConnectionFailed) {
    TestForCaptivePortal(base::BindBlockArc(^(
        const captive_portal::CaptivePortalDetector::Results& results) {
      CaptivePortalStatus status =
          CaptivePortalMetricsTabHelper::CaptivePortalStatusFromDetectionResult(
              results);
      UMA_HISTOGRAM_ENUMERATION(kCaptivePortalSecureConnectionFailedHistogram,
                                static_cast<int>(status),
                                static_cast<int>(CaptivePortalStatus::COUNT));
    }));
  }
}
