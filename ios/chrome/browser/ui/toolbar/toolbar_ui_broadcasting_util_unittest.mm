// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/toolbar_ui_broadcasting_util.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/toolbar/test/test_toolbar_ui_state.h"
#import "ios/chrome/browser/ui/toolbar/test/toolbar_broadcast_test_util.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_ui.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test implementation of ToolbarUI.
@interface TestToolbarUI : NSObject<ToolbarUI>
@property(nonatomic, readonly) TestToolbarUIState* toolbarUIState;
@end

@implementation TestToolbarUI
@synthesize toolbarUIState = _toolbarUIState;

- (instancetype)init {
  if (self = [super init]) {
    _toolbarUIState = [[TestToolbarUIState alloc] init];
  }
  return self;
}

@end

// Tests that the ToolbarUIBroadcastingUtil functions successfully start
// and stop broadcasting toolbar properties.
TEST(ToolbarUIBroadcastingUtilTest, StartStop) {
  TestToolbarUI* ui = [[TestToolbarUI alloc] init];
  ChromeBroadcaster* broadcaster = [[ChromeBroadcaster alloc] init];
  VerifyToolbarUIBroadcast(ui.toolbarUIState, broadcaster, false);
  StartBroadcastingToolbarUI(ui, broadcaster);
  VerifyToolbarUIBroadcast(ui.toolbarUIState, broadcaster, true);
  StopBroadcastingToolbarUI(broadcaster);
  VerifyToolbarUIBroadcast(ui.toolbarUIState, broadcaster, false);
}
