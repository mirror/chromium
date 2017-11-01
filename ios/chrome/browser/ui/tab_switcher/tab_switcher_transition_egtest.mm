// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#import "ios/chrome/app/main_controller_private.h"
#include "ios/chrome/browser/chrome_switches.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"
#include "ios/chrome/browser/ui/tools_menu/tools_menu_constants.h"
#import "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#include "ios/chrome/test/earl_grey/accessibility_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/test/http_server/blank_page_response_provider.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabel;
using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::StaticTextWithAccessibilityLabelId;
using web::test::HttpServer;

namespace {

// Returns the GREYMatcher for the button to go to the non incognito panel in
// the tab switcher.
id<GREYMatcher> TabletTabSwitcherOpenTabsButton() {
  NSString* accessibility_label = l10n_util::GetNSStringWithFixup(
      IDS_IOS_TAB_SWITCHER_HEADER_NON_INCOGNITO_TABS);
  return grey_allOf(grey_accessibilityLabel(accessibility_label),
                    grey_accessibilityTrait(UIAccessibilityTraitCausesPageTurn),
                    nil);
}

// Returns the GREYMatcher for the incognito tabs button in the tablet tab
// switcher.
id<GREYMatcher> TabletTabSwitcherIncognitoTabsButton() {
  NSString* accessibility_label = l10n_util::GetNSStringWithFixup(
      IDS_IOS_TAB_SWITCHER_HEADER_INCOGNITO_TABS);
  return grey_allOf(grey_accessibilityLabel(accessibility_label),
                    grey_accessibilityTrait(UIAccessibilityTraitCausesPageTurn),
                    nil);
}

// Returns the GREYMatcher for the button that creates new non incognito tabs
// from within the tablet tab switcher.
id<GREYMatcher> TabletTabSwitcherNewTabButton() {
  return grey_allOf(
      ButtonWithAccessibilityLabelId(IDS_IOS_TAB_SWITCHER_CREATE_NEW_TAB),
      grey_sufficientlyVisible(), nil);
}

// Returns the GREYMatcher for the button that creates new incognito tabs from
// within the tablet tab switcher.
id<GREYMatcher> TabletTabSwitcherNewIncognitoTabButton() {
  return grey_allOf(ButtonWithAccessibilityLabelId(
                        IDS_IOS_TAB_SWITCHER_CREATE_NEW_INCOGNITO_TAB),
                    grey_sufficientlyVisible(), nil);
}

TabModel* GetNormalTabModel() {
  return [[chrome_test_util::GetMainController() browserViewInformation]
      mainTabModel];
}

void ShowTabSwitcher() {
  id<GREYMatcher> matcher = nil;
  if (IsIPadIdiom()) {
    matcher =
        ButtonWithAccessibilityLabelId(IDS_IOS_TAB_STRIP_ENTER_TAB_SWITCHER);
  } else {
    matcher = chrome_test_util::ShowTabsButton();
  }
  DCHECK(matcher);

  [[EarlGrey selectElementWithMatcher:matcher] performAction:grey_tap()];
}

void ShowTabViewController() {
  id<GREYMatcher> matcher = nil;
  if (IsIPadIdiom()) {
    matcher =
        ButtonWithAccessibilityLabelId(IDS_IOS_TAB_STRIP_LEAVE_TAB_SWITCHER);
  } else {
    matcher = chrome_test_util::ShowTabsButton();
  }
  DCHECK(matcher);

  [[EarlGrey selectElementWithMatcher:matcher] performAction:grey_tap()];
}

}  // namespace

@interface AAATabSwitcherTransitionTestCase : ChromeTestCase
@end

// NOTE: The test cases before are not totally independent.  For example, the
// setup steps for testEnterTabSwitcherWithOneIncognitoTab first close the last
// normal tab and then open a new incognito tab, which are both scenarios
// covered by other tests.  A single programming error may cause multiple tests
// to fail.
@implementation AAATabSwitcherTransitionTestCase

// Tests entering the tab switcher when one normal tab is open.
- (void)testEnterSwitcherWithOneNormalTab {
  ShowTabSwitcher();
}

// Tests entering the tab switcher when more than one normal tab is open.
- (void)testEnterSwitcherWithMultipleNormalTabs {
  [ChromeEarlGreyUI openNewTab];
  [ChromeEarlGreyUI openNewTab];

  ShowTabSwitcher();
}

// Tests entering the tab switcher when one incognito tab is open.
- (void)testEnterSwitcherWithOneIncognitoTab {
  [ChromeEarlGreyUI openNewIncognitoTab];
  [GetNormalTabModel() closeAllTabs];

  ShowTabSwitcher();
}

// Tests entering the tab switcher when more than one incognito tab is open.
- (void)testEnterSwitcherWithMultipleIncognitoTabs {
  [ChromeEarlGreyUI openNewIncognitoTab];
  [GetNormalTabModel() closeAllTabs];
  [ChromeEarlGreyUI openNewIncognitoTab];
  [ChromeEarlGreyUI openNewIncognitoTab];

  ShowTabSwitcher();
}

// Tests entering the switcher when multiple tabs of both types are open.
- (void)testEnterSwitcherWithNormalAndIncognitoTabs {
  [ChromeEarlGreyUI openNewTab];
  [ChromeEarlGreyUI openNewIncognitoTab];
  [ChromeEarlGreyUI openNewIncognitoTab];

  ShowTabSwitcher();
}

// Tests entering the tab switcher by closing the last normal tab.
- (void)testEnterSwitcherByClosingLastNormalTab {
  chrome_test_util::CloseAllTabsInCurrentMode();
}

// Tests entering the tab switcher by closing the last incognito tab.
- (void)testEnterSwitcherByClosingLastIncognitoTab {
  [ChromeEarlGreyUI openNewIncognitoTab];
  [GetNormalTabModel() closeAllTabs];

  chrome_test_util::CloseAllTabsInCurrentMode();
}

- (void)testLeaveSwitcherWithSwitcherButton {
  ShowTabSwitcher();
  ShowTabViewController();
}

- (void)testLeaveSwitcherByOpeningNewNormalTab {
  // Enter the switcher and open a new tab using the new tab button.
  ShowTabSwitcher();
  if (IsIPadIdiom()) {
    [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewTabButton()]
        performAction:grey_tap()];
  } else {
    [[EarlGrey
        selectElementWithMatcher:chrome_test_util::ButtonWithAccessibilityLabel(
                                     @"New Tab")] performAction:grey_tap()];
  }

  if (!IsIPadIdiom()) {
    // On phone, enter the switcher again and open a new tab using the tools
    // menu.  This does not apply on tablet because there is no tools menu
    // in the switcher.
    ShowTabSwitcher();
    [ChromeEarlGreyUI openNewTab];
  }
}

- (void)testLeaveSwitcherByOpeningNewIncognitoTab {
  // Set up by creating a new incognito tab and closing all normal tabs.
  [ChromeEarlGreyUI openNewIncognitoTab];
  [GetNormalTabModel() closeAllTabs];

  // Enter the switcher and open a new incognito tab using the new tab button.
  ShowTabSwitcher();
  if (IsIPadIdiom()) {
    [[EarlGrey
        selectElementWithMatcher:TabletTabSwitcherNewIncognitoTabButton()]
        performAction:grey_tap()];
  } else {
    [[EarlGrey
        selectElementWithMatcher:chrome_test_util::ButtonWithAccessibilityLabel(
                                     @"New Incognito Tab")]
        performAction:grey_tap()];
  }

  if (!IsIPadIdiom()) {
    // On phone, enter the switcher again and open a new incognito tab using the
    // tools menu.  This does not apply on tablet because there is no tools menu
    // in the switcher.
    ShowTabSwitcher();
    [ChromeEarlGreyUI openNewIncognitoTab];
  }
}

- (void)testLeaveSwitcherByOpeningTabInOtherMode {
  // Go from normal mode to incognito mode.
  ShowTabSwitcher();
  if (IsIPadIdiom()) {
    // Switch to the incognito tabs panel by swiping right on the "Open Tabs"
    // button.
    [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenTabsButton()]
        performAction:[GREYActionBlock
                          actionWithName:@"Decrement"
                             constraints:nil
                            performBlock:^BOOL(id element,
                                               NSError* __strong* errorOrNil) {
                              [element accessibilityDecrement];
                              return YES;
                            }]];
    [[EarlGrey
        selectElementWithMatcher:TabletTabSwitcherNewIncognitoTabButton()]
        performAction:grey_tap()];
  } else {
    [ChromeEarlGreyUI openNewIncognitoTab];
  }

  // Go from incognito mode to normal mode.
  ShowTabSwitcher();
  if (IsIPadIdiom()) {
    // Switch to the open tabs panel by swiping left on the "Incognito Tabs"
    // button.
    [[EarlGrey selectElementWithMatcher:TabletTabSwitcherIncognitoTabsButton()]
        performAction:[GREYActionBlock
                          actionWithName:@"Increment"
                             constraints:nil
                            performBlock:^BOOL(id element,
                                               NSError* __strong* errorOrNil) {
                              [element accessibilityIncrement];
                              return YES;
                            }]];
    [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewTabButton()]
        performAction:grey_tap()];
  } else {
    [ChromeEarlGreyUI openNewTab];
  }
}

- (void)XX_testWithDelay {
  chrome_test_util::OpenNewTab();
  chrome_test_util::OpenNewTab();
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];

  GREYCondition* myCondition = [GREYCondition conditionWithName:@"delay"
                                                          block:^BOOL {
                                                            return NO;
                                                          }];
  // Wait for my condition to be satisfied or timeout after 5 seconds.
  //[myCondition waitWithTimeout:5];

  [[EarlGrey selectElementWithMatcher:chrome_test_util::ShowTabsButton()]
      performAction:grey_tap()];
  [myCondition waitWithTimeout:20];
}

@end
