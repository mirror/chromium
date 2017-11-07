// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONSTANTS_H_

#import <Foundation/Foundation.h>

@class ToolsMenuCoordinator, ToolsMenuConfiguration, UIButton;

// Notification that the tools menu will be shown.
extern NSString* const kToolsMenuWillShowNotification;
// Notification that the tools menu will dismiss.
extern NSString* const kToolsMenuWillHideNotification;
// Notification that the tools menu did show.
extern NSString* const kToolsMenuDidShowNotification;
// Notification that the tools menu did dismiss.
extern NSString* const kToolsMenuDidHideNotification;

// A protocol which allows details of the presentation of the tools menu to be
// configured.
@protocol ToolsMenuCoordinatorPresentationProvider<NSObject>
@optional
// Returns the button being used to present and dismiss the tools menu,
// if applicable.
- (UIButton*)presentingButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@end

// A protocol which allows objects to be able to request information about
// the presentation state of the tools UI without having to have full
// knowledge of the ToolsMenuCoordinator class.
@protocol ToolsMenuCoordinatorPresentationStateProtocol<NSObject>
// Returns whether the tools menu on screen and being presented to the user.
- (BOOL)isShowingToolsMenu;
@end

// A protocol that describes a set of methods which may configure a
// ToolsMenuCoordinator. Most of the configuration of the coordinator may be
// achieved through the required ToolsMenuConfiguration object, but some
// optional minor elements such as bookmark highlights are also independently
// configurable.
@protocol ToolsMenuCoordinatorConfigurationProvider<NSObject>
// Returns a ToolsMenuConfiguration object describing the desired configuration
// of the tools menu.
- (ToolsMenuConfiguration*)menuConfigurationForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@optional
// If implemented, may influence how the tools menu shows the bookmark
// indicator in the tools UI.
- (BOOL)shouldHighlightBookmarkButtonForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence the presence of the find bar in the tools
// UI.
- (BOOL)shouldShowFindBarForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented,may influence the presence of the share menu in the tools
// UI.
- (BOOL)shouldShowShareMenuForToolsMenuCoordinator:
    (ToolsMenuCoordinator*)coordinator;
// If implemented, may influence how the tools menu shows page-reload related
// UI.
- (BOOL)isTabLoadingForToolsMenuCoordinator:(ToolsMenuCoordinator*)coordinator;
// If implemented, allows the delegate to be informed of an imminent
// presentation of the tools menu. The delegate may choose to dismiss other
// presented UI or otherwise configure itself for the menu presentation.
- (void)prepareForToolsMenuPresentationByCoordinator:
    (ToolsMenuCoordinator*)coordinator;
@end

// New Tab item accessibility Identifier.
extern NSString* const kToolsMenuNewTabId;
// New incognito Tab item accessibility Identifier.
extern NSString* const kToolsMenuNewIncognitoTabId;
// Close all Tabs item accessibility Identifier.
extern NSString* const kToolsMenuCloseAllTabsId;
// Close all incognito Tabs item accessibility Identifier.
extern NSString* const kToolsMenuCloseAllIncognitoTabsId;
// Bookmarks item accessibility Identifier.
extern NSString* const kToolsMenuBookmarksId;
// Reading List item accessibility Identifier.
extern NSString* const kToolsMenuReadingListId;
// Other Devices item accessibility Identifier.
extern NSString* const kToolsMenuOtherDevicesId;
// History item accessibility Identifier.
extern NSString* const kToolsMenuHistoryId;
// Report an issue item accessibility Identifier.
extern NSString* const kToolsMenuReportAnIssueId;
// Find in Page item accessibility Identifier.
extern NSString* const kToolsMenuFindInPageId;
// Request desktop item accessibility Identifier.
extern NSString* const kToolsMenuRequestDesktopId;
// Settings item accessibility Identifier.
extern NSString* const kToolsMenuSettingsId;
// Help item accessibility Identifier.
extern NSString* const kToolsMenuHelpId;
// Request mobile item accessibility Identifier.
extern NSString* const kToolsMenuRequestMobileId;

// Identifiers for tools menu items (for metrics purposes).
typedef NS_ENUM(int, ToolsMenuItemID) {
  // All of these values must be < 0.
  TOOLS_STOP_ITEM = -1,
  TOOLS_RELOAD_ITEM = -2,
  TOOLS_BOOKMARK_ITEM = -3,
  TOOLS_BOOKMARK_EDIT = -4,
  TOOLS_SHARE_ITEM = -5,
  TOOLS_MENU_ITEM = -6,
  TOOLS_SETTINGS_ITEM = -7,
  TOOLS_NEW_TAB_ITEM = -8,
  TOOLS_NEW_INCOGNITO_TAB_ITEM = -9,
  TOOLS_READING_LIST = -10,
  // -11 is deprecated.
  TOOLS_SHOW_HISTORY = -12,
  TOOLS_CLOSE_ALL_TABS = -13,
  TOOLS_CLOSE_ALL_INCOGNITO_TABS = -14,
  TOOLS_VIEW_SOURCE = -15,
  TOOLS_REPORT_AN_ISSUE = -16,
  TOOLS_SHOW_FIND_IN_PAGE = -17,
  TOOLS_SHOW_HELP_PAGE = -18,
  TOOLS_SHOW_BOOKMARKS = -19,
  TOOLS_SHOW_RECENT_TABS = -20,
  TOOLS_REQUEST_DESKTOP_SITE = -21,
  TOOLS_REQUEST_MOBILE_SITE = -22,
};

#endif  // IOS_CHROME_BROWSER_UI_TOOLS_MENU_TOOLS_MENU_CONSTANTS_H_
