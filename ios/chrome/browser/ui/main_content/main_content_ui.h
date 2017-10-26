// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_H_
#define IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_H_

#import <Foundation/Foundation.h>

@class MainContentUIForwarder;

// Protocol for the UI displaying the main content area.
@protocol MainContentUI<NSObject>

// The UI forwarder.
@property(nonatomic, readonly) MainContentUIForwarder* UIForwarder;

@end

// Convenience typedef for UIViewControllers displaying main content.
@class UIViewController;
typedef UIViewController<MainContentUI> MainContentViewController;

#endif  // IOS_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_UI_H_
