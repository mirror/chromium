// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_

#import "ios/shared/chrome/browser/ui/coordinators/browser_coordinator.h"

class OverlayQueue;

// A BrowserCoordinator that can be presented with OverlayService.  This class
@interface OverlayCoordinator : BrowserCoordinator

// Called when the coordinator was added to |queue|.  An OverlayCoordinator is
// expected to only be added to one OverlayQueue for in its lifetime.
- (void)wasAddedToQueue:(OverlayQueue*)queue;

// Starts overlaying this coordinator over |overlayParent|.  The receiver will
// be added as a child of |overlayParent|.
- (void)startOverlayingCoordinator:(BrowserCoordinator*)overlayParent;

// Performs cleanup tasks for the overlay.  This allows for deterministic
// cleanup to occur for coordinators whose UI has not been started. Rather than
// relying on |-dealloc| to perform cleanup, |-cancelOverlay| can be used to
// perform cleanup tasks deterministically.
- (void)cancel;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_COORDINATOR_H_
