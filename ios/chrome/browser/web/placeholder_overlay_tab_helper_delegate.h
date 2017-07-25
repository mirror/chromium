// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class PlaceholderOverlayTabHelper;
@class UIImage;
@class UIView;

// Delegate for PlaceholderOverlayTabHelper.
@protocol PlaceholderOverlayTabHelperDelegate<NSObject>

// Synchronously or asynchronously provides an image to cover what WebState is
// actually displaying.
- (void)placeholderOverlayTabHelper:(PlaceholderOverlayTabHelper*)tabHelper
      getImageWithCompletionHandler:(void (^)(UIImage*))completionHandler;

// Displays the view to cover what WebState is actually displaying.
- (void)placeholderOverlayTabHelper:(PlaceholderOverlayTabHelper*)tabHelper
                 displayOverlayView:(UIView*)view;

@end

#endif  // IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_DELEGATE_H_
