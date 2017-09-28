// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_SUBCLASSING_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_SUBCLASSING_H_

#import "ios/clean/chrome/browser/ui/main_content/main_content_coordinator.h"

@class MainContentViewController;

// Interface used to expose MainContentCoordinator configuration to subclasses.
@interface MainContentCoordinator (MainContentSubclassing)

// The MainContentViewController used to display the page's content.  This
// should be created in a MainContentCoordinator subclass's |-start|.
@property(nonatomic, strong, readonly)
    MainContentViewController* contentViewController;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_SUBCLASSING_H_
