// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/overlay_coordinator.h"

#include "base/logging.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_queue.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_service_factory.h"
#import "ios/shared/chrome/browser/ui/browser_list/browser.h"
#import "ios/shared/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OverlayCoordinator ()
// The queue that presented this overlay.
@property(nonatomic, assign) OverlayQueue* queue;
@end

@implementation OverlayCoordinator
@synthesize queue = _queue;

#pragma mark - Public

- (void)wasAddedToQueue:(OverlayQueue*)queue {
  DCHECK(!self.queue);
  DCHECK(queue);
  self.queue = queue;
}

- (void)startOverlayingCoordinator:(BrowserCoordinator*)overlayParent {
  DCHECK(self.queue);
  DCHECK(overlayParent);
  [overlayParent addChildCoordinator:self];
  [self start];
}

- (void)cancel {
  // Subclasses implement.
}

#pragma mark - BrowserCoordinator

- (void)stop {
  DCHECK(self.queue);
  DCHECK(self.parentCoordinator);
  [super stop];
  [self.parentCoordinator removeChildCoordinator:self];
  self.queue->OverlayWasStopped(self);
}

@end
