// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_DELEGATE_H_
#define IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_DELEGATE_H_

#import <Foundation/Foundation.h>

class PagePlaceholderTabHelper;
@class UIImage;
@class UIView;

// Delegate for PagePlaceholderTabHelper.
@protocol PagePlaceholderTabHelperDelegate<NSObject>

// Synchronously or asynchronously provides an image to cover what WebState is
// actually displaying.
- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
    getImageWithCompletionHandler:(void (^)(UIImage*))completionHandler;

// Displays the view to cover what WebState is actually displaying.
- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
          displayPlaceholderView:(UIView*)view;

// Removes the view that covers what WebState is actually displaying.
- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
           removePlaceholderView:(UIView*)view;

@end

#endif  // IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_DELEGATE_H_
