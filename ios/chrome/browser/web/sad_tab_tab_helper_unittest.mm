// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/sad_tab_tab_helper.h"

#import "ios/chrome/browser/web/sad_tab_tab_helper_delegate.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#import "ios/web/public/web_state/ui/crw_generic_content_view.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Delegate for testing.
@interface SadTabTabHelperTestDelegate : NSObject<SadTabTabHelperDelegate>
@property(nonatomic, assign) BOOL sadTabShown;
@end

@implementation SadTabTabHelperTestDelegate
@synthesize sadTabShown = _sadTabShown;

- (void)sadTabHelper:(SadTabTabHelper*)tabHelper
    presentSadTabForRepeatedFailure:(BOOL)repeatedFailure {
  self.sadTabShown = YES;
}

@end

class SadTabTabHelperTest : public PlatformTest {
 protected:
  SadTabTabHelperTest()
      : application_(OCMClassMock([UIApplication class])),
        delegate_([[SadTabTabHelperTestDelegate alloc] init]) {
    SadTabTabHelper::CreateForWebState(&web_state_, delegate_);
    OCMStub([application_ sharedApplication]).andReturn(application_);
  }
  ~SadTabTabHelperTest() override { [application_ stopMocking]; }
  web::TestWebState web_state_;
  id application_;
  SadTabTabHelperTestDelegate* delegate_;
};

// Tests that SadTab is not presented for not shown web states.
TEST_F(SadTabTabHelperTest, NotPresented) {
  OCMStub([application_ applicationState]).andReturn(UIApplicationStateActive);
  web_state_.WasHidden();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(delegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because web state was not shown.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(delegate_.sadTabShown);
}

// Tests that SadTab is not presented if app is in background.
TEST_F(SadTabTabHelperTest, AppInBackground) {
  OCMStub([application_ applicationState])
      .andReturn(UIApplicationStateBackground);
  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(delegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because application is backgrounded.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(delegate_.sadTabShown);
}

// Tests that SadTab is not presented if app is in inactive.
TEST_F(SadTabTabHelperTest, AppIsInactive) {
  OCMStub([application_ applicationState])
      .andReturn(UIApplicationStateInactive);
  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(delegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because application is inactive.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(delegate_.sadTabShown);
}

// Tests that SadTab is presented for shown web states.
TEST_F(SadTabTabHelperTest, Presented) {
  OCMStub([application_ applicationState]).andReturn(UIApplicationStateActive);

  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(delegate_.sadTabShown);

  // Helper should get notified of render process failure. And the delegate
  // should present a SadTab.
  web_state_.OnRenderProcessGone();
  EXPECT_TRUE(delegate_.sadTabShown);
}

