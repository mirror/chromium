// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// The view controller used to display the main content of the Browser.
// Coordinators that display the UI for the current URL should use
// MainContentViewControllers.
@interface MainContentViewController : UIViewController

// The vertical content offset for the main content's UIScrollView.
@property(nonatomic, readonly) CGFloat yContentOffset;
// Whether the main content's UIScrollView is scrolling.  This is YES when the
// main content's UIScrollView is dragging or decelerating.
@property(nonatomic, readonly, getter=isScrolling) BOOL scrolling;
// Whether the main content's UIScrollView is being dragged.
@property(nonatomic, readonly, getter=isDragging) BOOL dragging;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_VIEW_CONTROLLER_H_
