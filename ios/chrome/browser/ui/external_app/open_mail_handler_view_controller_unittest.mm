// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_app/open_mail_handler_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_url_rewriter.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest_mac.h"
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

  // Returns an OCMock object representing a MailtoHandler object with name
  // |appName| and ID of |appID|. The app will be listed with availability of
  // |available|. Returns as id so further expect/stub can be done. Caller
  // should cast it to the proper type if necessary.
  id CreateMockApp(NSString* appName, NSString* appID, BOOL available) {
    id mailApp = OCMClassMock([MailtoHandler class]);
    OCMStub([mailApp appName]).andReturn(appName);
    OCMStub([mailApp appStoreID]).andReturn(appID);
    OCMStub([mailApp isAvailable]).andReturn(available);
    return mailApp;
  }

  MailtoURLRewriter* rewriter_;
};

// Verifies the basic structure of the model.
TEST_F(OpenMailHandlerViewControllerTest, TestModel) {
  // Mock rewriter_ must be created before the controller.
  id rewriterMock = OCMClassMock([MailtoURLRewriter class]);
  NSArray* mailApps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", NO),
    CreateMockApp(@"app3", @"333", YES)
  ];
  OCMStub([rewriterMock defaultHandlers]).andReturn(mailApps);
  rewriter_ = base::mac::ObjCCastStrict<MailtoURLRewriter>(rewriterMock);
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
}

// Verifies that when the second row is selected, the second mailto:
TEST_F(OpenMailHandlerViewControllerTest, TestSelectionCallback) {
  // Mock rewriter_ must be created before the controller.
  id rewriterMock = OCMClassMock([MailtoURLRewriter class]);
  NSArray* mailApps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", YES)
  ];
  OCMStub([rewriterMock defaultHandlers]).andReturn(mailApps);
  rewriter_ = base::mac::ObjCCastStrict<MailtoURLRewriter>(rewriterMock);
  CreateController();
  OpenMailHandlerViewController* testViewController =
      base::mac::ObjCCastStrict<OpenMailHandlerViewController>(controller());

  // Tests that when the second app which has an ID of @"222" is selected, the
  // callback block is called with the correct |handler|.
  NSIndexPath* selectedIndexPath = [NSIndexPath indexPathForRow:1 inSection:1];
  testViewController.onMailtoHandlerSelected = ^(MailtoHandler* handler) {
    EXPECT_NSEQ(@"222", [handler appStoreID]);
  };
  [testViewController collectionView:[testViewController collectionView]
            didSelectItemAtIndexPath:selectedIndexPath];
}

TEST_F(OpenMailHandlerViewControllerTest, TestAlwaysAskToggle) {
  // Mock rewriter_ must be created before the controller.
  id rewriterMock = OCMClassMock([MailtoURLRewriter class]);
  NSArray* mailApps = @[
    CreateMockApp(@"app1", @"111", YES), CreateMockApp(@"app2", @"222", YES)
  ];
  OCMStub([rewriterMock defaultHandlers]).andReturn(mailApps);
  // The second app will be selected for this test.
  OCMExpect([rewriterMock setDefaultHandlerID:@"222"]);
  rewriter_ = base::mac::ObjCCastStrict<MailtoURLRewriter>(rewriterMock);
  CreateController();

  // Finds the UISwitch cell and toggle the switch to OFF.
  OpenMailHandlerViewController* testViewController =
      base::mac::ObjCCastStrict<OpenMailHandlerViewController>(controller());
  NSIndexPath* toggleIndexPath = [NSIndexPath indexPathForRow:0 inSection:2];
  CollectionViewSwitchCell* toggleCell =
      base::mac::ObjCCastStrict<CollectionViewSwitchCell>([testViewController
                  collectionView:[testViewController collectionView]
          cellForItemAtIndexPath:toggleIndexPath]);
  toggleCell.switchView.on = NO;
  [toggleCell.switchView
      sendActionsForControlEvents:UIControlEventValueChanged];

  // Then selects the second app in the list of apps.
  NSIndexPath* selectedIndexPath = [NSIndexPath indexPathForRow:1 inSection:1];
  [testViewController collectionView:[testViewController collectionView]
            didSelectItemAtIndexPath:selectedIndexPath];
  EXPECT_OCMOCK_VERIFY(rewriterMock);
}
