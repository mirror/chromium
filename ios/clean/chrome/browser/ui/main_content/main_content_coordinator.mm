// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/main_content/main_content_coordinator.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcast_observer.h"
#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_coordinator+subclassing.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MainContentCoordinator

- (void)start {
  DCHECK(self.contentViewController);
  DCHECK_EQ(self.viewController, self.contentViewController);
  [self.browser->broadcaster()
      broadcastValue:@"yContentOffset"
            ofObject:self.contentViewController
            selector:@selector(broadcastContentScrollOffset:)];
  [self.browser->broadcaster()
      broadcastValue:@"scrolling"
            ofObject:self.contentViewController
            selector:@selector(broadcastScrollViewIsScrolling:)];
  [self.browser->broadcaster()
      broadcastValue:@"dragging"
            ofObject:self.contentViewController
            selector:@selector(broadcastScrollViewIsDragging:)];
  [super start];
}

- (void)stop {
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastContentScrollOffset:)];
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsScrolling:)];
  [self.browser->broadcaster()
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsDragging:)];
  [super stop];
}

@end

@implementation MainContentCoordinator (Internal)

- (UIViewController*)viewController {
  return self.contentViewController;
}

@end

@implementation MainContentCoordinator (MainContentSubclassing)

- (MainContentViewController*)contentViewController {
  // Subclasses implement.
  NOTREACHED();
  return nil;
}

@end
