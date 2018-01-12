// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_home_tablet_ntp_controller.h"

#include <memory>

#include "base/ios/block_types.h"
#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/bookmarks_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/metrics/new_tab_page_uma.h"
#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_collection_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_editor_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_primary_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller_protected.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_waiting_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_item.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_navigation_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_panel_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_promo_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#include "ios/chrome/browser/ui/main/main_feature_flags.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/web/public/referrer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/page_transition_types.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkNode;

@interface BookmarkHomeTabletNTPController ()<BookmarkMenuViewDelegate>

#pragma mark View loading, laying out, and switching.

// Returns whether the menu should be in a side panel that slides in.
- (BOOL)shouldPresentMenuInSlideInPanel;
// Updates the frame of the folder view.
- (void)refreshFrameOfFolderView;
// Returns the frame of the folder view.
- (CGRect)frameForFolderView;

// The menu button is pressed on the editing bar.
- (void)toggleMenuAnimated;

@end

@implementation BookmarkHomeTabletNTPController
// Property declared in NewTabPagePanelProtocol.
@synthesize delegate = _delegate;

#pragma mark - UIViewController

- (void)viewWillLayoutSubviews {
  [super viewWillLayoutSubviews];

  // Store the content scroll position.
  CGFloat contentPosition =
      [[self folderView] contentPositionInPortraitOrientation];
  // If we have the cached position, use it instead.
  if (self.cachedContentPosition) {
    contentPosition = [self.cachedContentPosition floatValue];
    self.cachedContentPosition = nil;
  }

  if (!self.folderView && ![self primaryMenuItem] && self.bookmarks->loaded()) {
    BookmarkMenuItem* item = nil;
    CGFloat position = 0;
    BOOL found =
        bookmark_utils_ios::GetPositionCache(self.bookmarks, &item, &position);
    if (!found)
      item = [self.menuView defaultMenuItem];

    [self updatePrimaryMenuItem:item animated:NO];
  }
  
  UIInterfaceOrientation orient = GetInterfaceOrientation();
  [self refreshFrameOfFolderView];
  [self.folderView changeOrientation:orient];
  if (![self shouldPresentMenuInSlideInPanel])
    [self updateMenuViewLayout];

  // Restore the content scroll position if it was reset to zero. This could
  // happen when folderView is newly created (restore from cached); its frame
  // height has changed; or it was re-attached to the view hierarchy.
  if (contentPosition > 0 &&
      [[self folderView] contentPositionInPortraitOrientation] == 0) {
    [[self folderView] applyContentPosition:contentPosition];
  }
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.view.backgroundColor = bookmark_utils_ios::mainBackgroundColor();

  if (self.bookmarks->loaded())
    [self loadBookmarkViews];
  else
    [self loadWaitingView];
}

#pragma mark - Views

- (void)updateMenuViewLayout {
  LayoutRect menuLayout =
      LayoutRectMake(0, self.view.bounds.size.width, 0, self.menuWidth,
                     self.view.bounds.size.height);
  self.menuView.frame = LayoutRectGetRect(menuLayout);
}

#pragma mark - Superclass overrides

- (void)loadBookmarkViews {
  [super loadBookmarkViews];
  DCHECK(self.bookmarks->loaded());

  self.menuView.delegate = self;

  [self.view addSubview:self.menuView];
  [self.view addSubview:self.folderView];

  // Load the last primary menu item which the user had active.
  BookmarkMenuItem* item = nil;
  CGFloat position = 0;
  BOOL found =
      bookmark_utils_ios::GetPositionCache(self.bookmarks, &item, &position);
  if (!found)
    item = [self.menuView defaultMenuItem];

  [self updatePrimaryMenuItem:item animated:NO];

  if (found) {
    // If the view has already been laid out, then immediately apply the content
    // position.
    if (self.view.window) {
      [self.folderView applyContentPosition:position];
    } else {
      // Otherwise, save the position to be applied once the view has been laid
      // out.
      self.cachedContentPosition = [NSNumber numberWithFloat:position];
    }
  }
}

- (void)updatePrimaryMenuItem:(BookmarkMenuItem*)menuItem
                     animated:(BOOL)animated {
  [super updatePrimaryMenuItem:menuItem animated:animated];

  [self refreshFrameOfFolderView];
}

- (ActionSheetCoordinator*)createActionSheetCoordinatorOnView:(UIView*)view {
  UIViewController* baseViewController =
      TabSwitcherPresentsBVCEnabled() ? self
                                      : self.view.window.rootViewController;

  return [[ActionSheetCoordinator alloc]
      initWithBaseViewController:baseViewController
                           title:nil
                         message:nil
                            rect:view.bounds
                            view:view];
}

#pragma mark - Private methods

- (BOOL)shouldPresentMenuInSlideInPanel {
  return IsCompactTablet();
}

- (void)refreshFrameOfFolderView {
  self.folderView.frame = [self frameForFolderView];
}

- (CGRect)frameForFolderView {
  CGFloat topInset = 0;

  CGFloat leadingMargin = 0;
  CGSize size = self.view.bounds.size;
  LayoutRect folderViewLayout =
      LayoutRectMake(leadingMargin, size.width, topInset,
                     size.width - leadingMargin, size.height - topInset);
  return LayoutRectGetRect(folderViewLayout);
}

#pragma mark - BookmarkMenuViewDelegate

- (void)bookmarkMenuView:(BookmarkMenuView*)view
        selectedMenuItem:(BookmarkMenuItem*)menuItem {
  BOOL menuItemChanged = ![[self primaryMenuItem] isEqual:menuItem];
  [self toggleMenuAnimated];
  if (menuItemChanged) {
    [self updatePrimaryMenuItem:menuItem animated:YES];
  }
}

- (void)toggleMenuAnimated {
  if ([self.panelView userDrivenAnimationInProgress])
    return;

  if (self.panelView.showingMenu) {
    [self.panelView hideMenuAnimated:YES];
  } else {
    [self.panelView showMenuAnimated:YES];
  }
}

- (BOOL)shouldShowEditButton {
  if (self.primaryMenuItem.type != bookmarks::MenuItemFolder)
    return NO;
  // The type is MenuItemFolder, so it is safe to access |folder|.
  return !self.bookmarks->is_permanent_node(self.primaryMenuItem.folder);
}

#pragma mark - NewTabPagePanelProtocol

- (void)reload {
}

- (void)wasShown {
  [self.folderView wasShown];
}

- (void)wasHidden {
  [self cachePosition];
  [self.folderView wasHidden];
}

- (void)dismissModals {
  [super dismissModals];
  [self.editViewController dismiss];
}

- (void)dismissKeyboard {
  // Uses self.view directly instead of going through self.view to
  // avoid creating the view hierarchy unnecessarily.
  [self.view endEditing:YES];
}

- (void)setScrollsToTop:(BOOL)enabled {
  self.scrollToTop = enabled;
  [self.folderView setScrollsToTop:self.scrollToTop];
}

- (CGFloat)alphaForBottomShadow {
  return 0;
}

@end
