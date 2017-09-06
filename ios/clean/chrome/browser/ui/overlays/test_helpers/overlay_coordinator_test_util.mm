// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/test_helpers/overlay_coordinator_test_util.h"

#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_coordinator.h"
#import "ios/testing/wait_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace testing {

void WaitForOverlayPresentation(OverlayCoordinator* overlay) {
  ASSERT_TRUE(overlay);
  UIViewController* view_controller = overlay.viewController;
  EXPECT_TRUE(view_controller);
  EXPECT_TRUE(WaitUntilConditionOrTimeout(kWaitForUIElementTimeout, ^bool {
    return view_controller.presentingViewController &&
           !view_controller.beingPresented;
  }));
}

}  // namespace testing
