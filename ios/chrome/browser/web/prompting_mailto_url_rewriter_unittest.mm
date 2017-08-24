// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/prompting_mailto_url_rewriter.h"

#import "ios/chrome/browser/web/fake_mailto_handler_helpers.h"
#import "ios/chrome/browser/web/mailto_handler_system_mail.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface PromptingMailtoURLRewriter (Testing)
- (void)addMailtoApps:(NSArray<MailtoHandler*>*)handlerApps;
@end

class PromptingMailtoURLRewriterTest : public PlatformTest {
 protected:
  void SetUp() override {}
};

TEST_F(PromptingMailtoURLRewriterTest, TestStandardInstance) {
  PromptingMailtoURLRewriter* rewriter =
      [[PromptingMailtoURLRewriter alloc] initWithStandardHandlers];
  EXPECT_TRUE(rewriter);

  NSArray<MailtoHandler*>* handlers = [rewriter defaultHandlers];
  EXPECT_GE([handlers count], 1U);
  for (MailtoHandler* handler in handlers) {
    ASSERT_TRUE(handler);
    NSString* appStoreID = [handler appStoreID];
    NSString* expectedDefaultAppID =
        [handler isAvailable] ? appStoreID : [MailtoURLRewriter systemMailApp];
    [rewriter setDefaultHandlerID:appStoreID];
    EXPECT_NSEQ(expectedDefaultAppID, [rewriter defaultHandlerID]);
  }
}

// Case 1: If Gmail is not installed, rewriter defaults to system Mail app.
TEST_F(PromptingMailtoURLRewriterTest, TestNoGmailInstalled) {
  PromptingMailtoURLRewriter* rewriter =
      [[PromptingMailtoURLRewriter alloc] init];
  [rewriter addMailtoApps:@[
    [[MailtoHandlerSystemMail alloc] init],
    [[FakeMailtoHandlerGmailNotInstalled alloc] init]
  ]];
  EXPECT_NSEQ([MailtoURLRewriter systemMailApp], [rewriter defaultHandlerID]);
}

// Case 2: If Gmail is installed but user has not made a choice, there is
// no default mail app.
TEST_F(PromptingMailtoURLRewriterTest, TestWithGmailChoiceNotMade) {
  PromptingMailtoURLRewriter* rewriter =
      [[PromptingMailtoURLRewriter alloc] init];
  [rewriter addMailtoApps:@[
    [[MailtoHandlerSystemMail alloc] init],
    [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  EXPECT_FALSE([rewriter defaultHandlerID]);
}

// Case 3: If Gmail was installed and user has made a choice, then Gmail is
// uninstalled. The default returns to system Mail app.
TEST_F(PromptingMailtoURLRewriterTest, TestWithGmailUninstalled) {
  PromptingMailtoURLRewriter* rewriter =
      [[PromptingMailtoURLRewriter alloc] init];
  MailtoHandler* systemMailHandler = [[MailtoHandlerSystemMail alloc] init];
  MailtoHandler* fakeGmailHandler =
      [[FakeMailtoHandlerGmailInstalled alloc] init];
  [rewriter addMailtoApps:@[ systemMailHandler, fakeGmailHandler ]];
  [rewriter setDefaultHandlerID:[fakeGmailHandler appStoreID]];
  EXPECT_NSEQ([fakeGmailHandler appStoreID], [rewriter defaultHandlerID]);

  rewriter = [[PromptingMailtoURLRewriter alloc] init];
  fakeGmailHandler = [[FakeMailtoHandlerGmailNotInstalled alloc] init];
  [rewriter addMailtoApps:@[ systemMailHandler, fakeGmailHandler ]];
  EXPECT_NSEQ([MailtoURLRewriter systemMailApp], [rewriter defaultHandlerID]);
}

// Case 4: If Gmail is installed but system Mail app has been chosen by
// user as the default mail handler app. Then Gmail is uninstalled. User's
// choice of system Mail app remains unchanged and will persist through a
// re-installation of Gmail.
TEST_F(PromptingMailtoURLRewriterTest,
       TestSystemMailAppChosenSurviveGmailUninstall) {
  // Initial state of system Mail app explicitly chosen.
  PromptingMailtoURLRewriter* rewriter =
      [[PromptingMailtoURLRewriter alloc] init];
  MailtoHandler* systemMailHandler = [[MailtoHandlerSystemMail alloc] init];
  [rewriter addMailtoApps:@[
    systemMailHandler, [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  [rewriter setDefaultHandlerID:[systemMailHandler appStoreID]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [rewriter defaultHandlerID]);

  // Gmail is installed.
  rewriter = [[PromptingMailtoURLRewriter alloc] init];
  [rewriter addMailtoApps:@[
    systemMailHandler, [[FakeMailtoHandlerGmailNotInstalled alloc] init]
  ]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [rewriter defaultHandlerID]);

  // Gmail is installed again.
  rewriter = [[PromptingMailtoURLRewriter alloc] init];
  [rewriter addMailtoApps:@[
    systemMailHandler, [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [rewriter defaultHandlerID]);
}
