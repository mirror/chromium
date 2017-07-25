// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/reading_list_menu_view_item.h"

#import <UIKit/UIKit.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ReadingListMenuViewCell (Testing)

- (BOOL)isNumberBadgeHidden;
- (BOOL)isTextBadgeHidden;
- (BOOL)isProperty;

@end

namespace {

class ReadingListMenuViewCellTest : public PlatformTest {
 public:
  ReadingListMenuViewCell* getReadingListMenuViewCell() {
    return readingListMenuViewCell_;
  }

 protected:
  void SetUp() override {
    readingListMenuViewCell_ = [[ReadingListMenuViewCell alloc] init];
    [readingListMenuViewCell_ initializeViews];
  }
  ReadingListMenuViewCell* readingListMenuViewCell_;
};

TEST_F(ReadingListMenuViewCellTest, Initialization) {
  ASSERT_EQ(getReadingListMenuViewCell().isNumberBadgeHidden, YES);
  ASSERT_EQ(getReadingListMenuViewCell().isTextBadgeHidden, YES);
}

TEST_F(ReadingListMenuViewCellTest, NumberBadgeAlone) {
  [getReadingListMenuViewCell() updateBadgeCount:1 animated:NO];

  ASSERT_EQ(getReadingListMenuViewCell().isNumberBadgeHidden, NO);
  ASSERT_EQ(getReadingListMenuViewCell().isTextBadgeHidden, YES);
}

TEST_F(ReadingListMenuViewCellTest, TextBadgeAlone) {
  [getReadingListMenuViewCell() updateShowTextBadge:YES animated:NO];

  ASSERT_EQ(getReadingListMenuViewCell().isNumberBadgeHidden, YES);
  ASSERT_EQ(getReadingListMenuViewCell().isTextBadgeHidden, NO);
}

TEST_F(ReadingListMenuViewCellTest, NumberBadgeHidesTextBadge) {
  [getReadingListMenuViewCell() updateShowTextBadge:YES animated:NO];
  [getReadingListMenuViewCell() updateBadgeCount:1 animated:NO];

  ASSERT_EQ(getReadingListMenuViewCell().isNumberBadgeHidden, NO);
  ASSERT_EQ(getReadingListMenuViewCell().isTextBadgeHidden, YES);
}

TEST_F(ReadingListMenuViewCellTest,
       TextBadgeDoesNotShowIfNumberBadgeIsVisible) {
  [getReadingListMenuViewCell() updateBadgeCount:1 animated:NO];
  [getReadingListMenuViewCell() updateShowTextBadge:YES animated:NO];

  ASSERT_EQ(getReadingListMenuViewCell().isNumberBadgeHidden, NO);
  ASSERT_EQ(getReadingListMenuViewCell().isTextBadgeHidden, YES);
}

}  // namespace
