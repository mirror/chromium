// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/external_app_launcher.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ExternalAppLauncher (Private)
// Returns the Phone/FaceTime call argument from |url|.
// Made available for unit testing.
+ (NSString*)formatCallArgument:(NSURL*)url;

// Exposed for unit testing to be stubbed so now prompt will open.
- (void)showExternalAppLauncherPrompt:(NSString*)prompt
                          acceptLabel:(NSString*)acceptLabel
                          rejectLabel:(NSString*)rejectLabel
                      responseHandler:(void (^)(BOOL response))responseHandler;

// Exposed for unit testing to be stubbed so no application is opened.
- (BOOL)openURL:(const GURL&)url linkClicked:(BOOL)linkClicked;

// The external App launcher doesn't open appliactions unless the prompt is not
// active, and because the test doesn't open a prompt this needs to be reset
// manually.
@property(nonatomic, getter=isPromptActive) BOOL promptActive;

@end

namespace {

using ExternalAppLauncherTest = PlatformTest;

TEST_F(ExternalAppLauncherTest, TestBadFormatCallArgument) {
  EXPECT_NSEQ(@"garbage:",
              [ExternalAppLauncher
                  formatCallArgument:[NSURL URLWithString:@"garbage:"]]);
  EXPECT_NSEQ(@"malformed:////",
              [ExternalAppLauncher
                  formatCallArgument:[NSURL URLWithString:@"malformed:////"]]);
}

TEST_F(ExternalAppLauncherTest, TestFormatCallArgument) {
  EXPECT_NSEQ(
      @"+1234",
      [ExternalAppLauncher
          formatCallArgument:[NSURL URLWithString:@"facetime://+1234"]]);
  EXPECT_NSEQ(
      @"+abcd",
      [ExternalAppLauncher
          formatCallArgument:[NSURL URLWithString:@"facetime-audio://+abcd"]]);
  EXPECT_NSEQ(
      @"+1-650-555-1212",
      [ExternalAppLauncher
          formatCallArgument:[NSURL URLWithString:@"tel://+1-650-555-1212"]]);
  EXPECT_NSEQ(@"75009",
              [ExternalAppLauncher
                  formatCallArgument:[NSURL URLWithString:@"garbage:75009"]]);
}

TEST_F(ExternalAppLauncherTest, TestURLEscapedArgument) {
  EXPECT_NSEQ(@"+1 650 555 1212",
              [ExternalAppLauncher
                  formatCallArgument:
                      [NSURL URLWithString:@"tel://+1%20650%20555%201212"]]);
}

TEST_F(ExternalAppLauncherTest, TestRepeatedAppLaunches) {
  // Setup.
  ExternalAppLauncher* launcher = [[ExternalAppLauncher alloc] init];
  GURL fromURL1("http://www.google.com");
  GURL fromURL2("https://www.goog.com");
  id launcherMock = [OCMockObject partialMockForObject:launcher];
  __block BOOL promptShown = NO;
  void (^updatePromptShown)(NSInvocation*) = ^(NSInvocation* invocation) {
    promptShown = YES;
  };
  [[[launcherMock stub] andDo:updatePromptShown]
      showExternalAppLauncherPrompt:[OCMArg any]
                        acceptLabel:[OCMArg any]
                        rejectLabel:[OCMArg any]
                    responseHandler:[OCMArg any]];

  // Test same source and same application
  [[[launcherMock expect] ignoringNonObjectArgs] openURL:GURL()
                                             linkClicked:YES];
  [launcher requestURLOpen:GURL("facetime://+1354")
                   fromURL:fromURL1
               linkClicked:YES];
  EXPECT_EQ(NO, promptShown);
  EXPECT_OCMOCK_VERIFY(launcherMock);

  [[[launcherMock expect] ignoringNonObjectArgs] openURL:GURL()
                                             linkClicked:YES];
  [launcher requestURLOpen:GURL("facetime://+12354")
                   fromURL:fromURL1
               linkClicked:YES];
  EXPECT_EQ(NO, promptShown);
  EXPECT_OCMOCK_VERIFY(launcherMock);

  [launcherMock requestURLOpen:GURL("facetime://+1236")
                       fromURL:fromURL1
                   linkClicked:YES];
  EXPECT_EQ(YES, promptShown);
  EXPECT_OCMOCK_VERIFY(launcherMock);

  // After prompt is show, the prompt status needs to be reset.
  promptShown = NO;
  launcher.promptActive = NO;

  // Different source URL
  [[[launcherMock expect] ignoringNonObjectArgs] openURL:GURL()
                                             linkClicked:YES];
  [launcher requestURLOpen:GURL("facetime://+1234")
                   fromURL:fromURL2
               linkClicked:YES];
  EXPECT_EQ(NO, promptShown);
  EXPECT_OCMOCK_VERIFY(launcherMock);

  // Different application
  [[[launcherMock expect] ignoringNonObjectArgs] openURL:GURL()
                                             linkClicked:YES];
  [launcher requestURLOpen:GURL("facetime-audio://+1234")
                   fromURL:fromURL1
               linkClicked:YES];
  EXPECT_EQ(NO, promptShown);
  EXPECT_OCMOCK_VERIFY(launcherMock);
}
}  // namespace
