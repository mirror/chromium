// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#import <Foundation/Foundation.h>

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

using CWVAutofillControllerTest = PlatformTest;

// Tests CWVAutofillController initialization.
TEST_F(CWVAutofillControllerTest, Initialization) {
  CWVAutofillController* autofillController =
      [[CWVAutofillController alloc] init];
  ASSERT_FALSE(autofillController.webState);
  ASSERT_FALSE(autofillController.autofillClient);
  ASSERT_FALSE(autofillController.autofillAgent);
  ASSERT_FALSE(autofillController.autofillManager);
  ASSERT_FALSE(autofillController.delegate);
  ASSERT_FALSE(autofillController.JSAutofillManager);
}

}  // namespace ios_web_view
