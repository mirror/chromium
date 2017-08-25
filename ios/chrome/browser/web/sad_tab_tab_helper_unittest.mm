// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/sad_tab_tab_helper.h"

#include <memory>

#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"
#import "ios/chrome/browser/web/page_placeholder_tab_helper_delegate.h"
#import "ios/chrome/browser/web/sad_tab_tab_helper_delegate.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
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
        sadTabdelegate_([[SadTabTabHelperTestDelegate alloc] init]),
        delegate_([OCMockObject
            mockForProtocol:@protocol(PagePlaceholderTabHelperDelegate)]) {
    SadTabTabHelper::CreateForWebState(&web_state_, sadTabdelegate_);
    PagePlaceholderTabHelper::CreateForWebState(&web_state_, delegate_);
    OCMStub([application_ sharedApplication]).andReturn(application_);

    // Setup navigation manager.
    std::unique_ptr<web::TestNavigationManager> navigation_manager =
        base::MakeUnique<web::TestNavigationManager>();
    navigation_manager->SetBrowserState(&browser_state_);
    navigation_manager_ = navigation_manager.get();
    web_state_.SetNavigationManager(std::move(navigation_manager));
  }

  ~SadTabTabHelperTest() override { [application_ stopMocking]; }
  web::TestBrowserState browser_state_;
  web::TestWebState web_state_;
  web::TestNavigationManager* navigation_manager_;
  id application_;
  SadTabTabHelperTestDelegate* sadTabdelegate_;
  id delegate_;
};

// Tests that SadTab is not presented for not shown web states and navigation
// item is reloaded once web state was shown.
TEST_F(SadTabTabHelperTest, ReloadedWhenWebStateWasShown) {
  OCMStub([application_ applicationState]).andReturn(UIApplicationStateActive);
  web_state_.WasHidden();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because web state was not shown.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Navigation item must be reloaded once web state is shown.
  EXPECT_FALSE(navigation_manager_->LoadIfNecessaryWasCalled());
  web_state_.WasShown();
  EXPECT_TRUE(PagePlaceholderTabHelper::FromWebState(&web_state_)
                  ->will_add_placeholder_for_next_navigation());
  EXPECT_TRUE(navigation_manager_->LoadIfNecessaryWasCalled());
}

// Tests that SadTab is not presented if app is in background and navigation
// item is reloaded once the app became active.
TEST_F(SadTabTabHelperTest, AppInBackground) {
  OCMStub([application_ applicationState])
      .andReturn(UIApplicationStateBackground);
  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because application is backgrounded.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Navigation item must be reloaded once the app became active.
  EXPECT_FALSE(navigation_manager_->LoadIfNecessaryWasCalled());
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIApplicationDidBecomeActiveNotification
                    object:nil];
  EXPECT_TRUE(PagePlaceholderTabHelper::FromWebState(&web_state_)
                  ->will_add_placeholder_for_next_navigation());
  EXPECT_TRUE(navigation_manager_->LoadIfNecessaryWasCalled());
}

// Tests that SadTab is not presented if app is in inactive  and navigation
// item is reloaded once the app became active.
TEST_F(SadTabTabHelperTest, AppIsInactive) {
  OCMStub([application_ applicationState])
      .andReturn(UIApplicationStateInactive);
  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Helper should get notified of render process failure,
  // but Sad Tab should not be presented, because application is inactive.
  web_state_.OnRenderProcessGone();
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Navigation item must be reloaded once the app became active.
  EXPECT_FALSE(navigation_manager_->LoadIfNecessaryWasCalled());
  [[NSNotificationCenter defaultCenter]
      postNotificationName:UIApplicationDidBecomeActiveNotification
                    object:nil];
  EXPECT_TRUE(PagePlaceholderTabHelper::FromWebState(&web_state_)
                  ->will_add_placeholder_for_next_navigation());
  EXPECT_TRUE(navigation_manager_->LoadIfNecessaryWasCalled());
}

// Tests that SadTab is presented for shown web states.
TEST_F(SadTabTabHelperTest, Presented) {
  OCMStub([application_ applicationState]).andReturn(UIApplicationStateActive);

  web_state_.WasShown();

  // Delegate should not present a SadTab.
  EXPECT_FALSE(sadTabdelegate_.sadTabShown);

  // Helper should get notified of render process failure. And the delegate
  // should present a SadTab.
  web_state_.OnRenderProcessGone();
  EXPECT_TRUE(sadTabdelegate_.sadTabShown);
}
