// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_H_

#import "ios/web/public/web_state/web_state_user_data.h"

namespace captive_portal {
class CaptivePortalDetector;
}

namespace web {
class WebState;
}

@protocol CaptivePortalDetectorTabHelperDelegate;

// Associates a Tab to a CaptivePortalDetector and manages its lifetime.
class CaptivePortalDetectorTabHelper
    : public web::WebStateUserData<CaptivePortalDetectorTabHelper> {
 public:
  ~CaptivePortalDetectorTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper and must not be
  // null.
  static void CreateForWebState(
      web::WebState* web_state,
      id<CaptivePortalDetectorTabHelperDelegate> delegate);

  // Returns the associated captive portal detector.
  captive_portal::CaptivePortalDetector* detector();

  // Returns the associated captive portal tab helper delegate.
  id<CaptivePortalDetectorTabHelperDelegate> delegate();

 private:
  CaptivePortalDetectorTabHelper(
      web::WebState* web_state,
      id<CaptivePortalDetectorTabHelperDelegate> delegate);

  // The underlying CaptivePortalDetector.
  std::unique_ptr<captive_portal::CaptivePortalDetector> detector_;

  // Delegate which displays and removes the placeholder to cover WebState's
  // view.
  __weak id<CaptivePortalDetectorTabHelperDelegate> delegate_ = nil;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalDetectorTabHelper);
};

#endif  // IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_H_
