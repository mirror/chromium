// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/main_content/main_content_coordinator.h"

#include "base/logging.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation MainContentCoordinator

- (MainContentViewController*)viewController {
  // Subclasses implement.
  NOTREACHED();
  return nil;
}

@end
