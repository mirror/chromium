// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/browser_coordinator+overlay_support.h"

#include "base/logging.h"
#import "ios/clean/chrome/browser/ui/overlays/internal/overlay_queue.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_service_factory.h"
#import "ios/shared/chrome/browser/ui/browser_list/browser.h"
#import "ios/shared/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BrowserCoordinator (OverlaySupport)
@dynamic overlayQueue;

- (OverlayService*)overlayService {
  return OverlayServiceFactory::GetInstance()->GetForBrowserState(
      self.browser->browser_state());
}

- (BOOL)supportsOverlaying {
  return NO;
}

- (void)startOverlayingCoordinator:(BrowserCoordinator*)overlayParent {
  DCHECK(overlayParent);
  [overlayParent addChildCoordinator:self];
  [self start];
}

- (void)overlayWasStopped {
  DCHECK(self.parentCoordinator);
  [self.parentCoordinator removeChildCoordinator:self];
  self.overlayQueue->OverlayWasStopped(self);
}

- (void)cancelOverlay {
  // Implemented by subclasses.
}

@end
