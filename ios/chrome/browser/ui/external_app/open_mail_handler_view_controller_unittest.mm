// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_app/open_mail_handler_view_controller.h"

#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_url_rewriter.h"
#include "ios/chrome/grit/ios_strings.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class OpenMailHandlerViewControllerTest : public CollectionViewControllerTest {
 protected:
  CollectionViewController* InstantiateController() override {
    return [[OpenMailHandlerViewController alloc] initWithRewriter:rewriter_];
  }

  MailtoURLRewriter* rewriter_;
};

TEST_F(OpenMailHandlerViewControllerTest, TestModel) {
  // Mocks a MailtoURLRewriter object with 3 mailto:// handler apps.
  // However, only 2 of them are actually available
  id mailApp1 = OCMClassMock([MailtoHandler class]);
  OCMStub([mailApp1 appName]).andReturn(@"app1");
  OCMStub([mailApp1 isAvailable]).andReturn(YES);
  id mailApp2 = OCMClassMock([MailtoHandler class]);
  OCMStub([mailApp2 appName]).andReturn(@"app2");
  OCMStub([mailApp2 isAvailable]).andReturn(NO);
  id mailApp3 = OCMClassMock([MailtoHandler class]);
  OCMStub([mailApp3 appName]).andReturn(@"app3");
  OCMStub([mailApp3 isAvailable]).andReturn(YES);
  id rewriterMock = OCMClassMock([MailtoURLRewriter class]);
  NSArray* mailApps = @[ mailApp1, mailApp2, mailApp3 ];
  OCMStub([rewriterMock defaultHandlers]).andReturn(mailApps);
  rewriter_ = static_cast<MailtoURLRewriter*>(rewriterMock);

  // Creates the controller.
  CreateController();
  CheckController();

  // Verifies the number of sections and items within each section.
  EXPECT_EQ(3, NumberOfSections());
  // Title
  EXPECT_EQ(1, NumberOfItemsInSection(0));
  // Apps. Only 2 out of 3 mailto:// handlers are actually availabel.
  EXPECT_EQ(2, NumberOfItemsInSection(1));
  // Footer with toggle option.
  EXPECT_EQ(1, NumberOfItemsInSection(2));
  CheckSwitchCellStateAndTitleWithId(YES, IDS_IOS_CHOOSE_EMAIL_ASK_TOGGLE, 2,
                                     0);

  // Checks the mocks.
  EXPECT_OCMOCK_VERIFY(rewriterMock);
  EXPECT_OCMOCK_VERIFY(mailApp1);
  EXPECT_OCMOCK_VERIFY(mailApp2);
}
