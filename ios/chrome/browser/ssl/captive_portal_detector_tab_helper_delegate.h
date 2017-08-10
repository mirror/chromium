// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class CaptivePortalDetectorTabHelper;

// Delegate for CaptivePortalDetectorTabHelper.
@protocol CaptivePortalDetectorTabHelperDelegate<NSObject>

// Notifies the delegate the user selected the connect button on the captive
// portal blocking page with the specified |landingURL|.
- (void)captivePortalTabHelper:(CaptivePortalDetectorTabHelper*)tabHelper
    didSelectConnectWithLandingURL:(NSURL*)landingURL;

@end

#endif  // IOS_CHROME_BROWSER_SSL_CAPTIVE_PORTAL_DETECTOR_TAB_HELPER_DELEGATE_H_
