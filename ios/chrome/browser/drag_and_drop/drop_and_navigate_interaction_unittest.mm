// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/drag_and_drop/drop_and_navigate_interaction.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

TEST(DropAndNavigateTest, Instantiation) {
  DropAndNavigateInteraction* interaction =
      [[DropAndNavigateInteraction alloc] initWithDelegate:nil];
}

}  // namespace
