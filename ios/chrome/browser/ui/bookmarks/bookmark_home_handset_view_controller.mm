// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_home_handset_view_controller.h"

#include <memory>

#include "base/logging.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/sys_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/bookmarks_utils.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_editor_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_primary_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller_protected.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_waiting_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_item.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_navigation_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_panel_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_promo_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_table_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/keyboard/UIKeyCommand+Chrome.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using bookmarks::BookmarkNode;

@interface BookmarkHomeHandsetViewController ()<BookmarkMenuViewDelegate,
                                                BookmarkPanelViewDelegate>

#pragma mark Private methods

// Returns the size of the panel view.
- (CGRect)frameForPanelView;
// Updates the UI to reflect the given orientation, with an animation lasting
// |duration|.
- (void)updateUIForInterfaceOrientation:(UIInterfaceOrientation)orientation
                               duration:(NSTimeInterval)duration;

@end

@implementation BookmarkHomeHandsetViewController

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher {
  self = [super initWithLoader:loader
                  browserState:browserState
                    dispatcher:dispatcher];
  return self;
}

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  if (self.bookmarks->loaded())
    [self loadBookmarkViews];
  else
    [self loadWaitingView];
}

// There are 2 UIViewController methods that could be overridden. This one and
// willAnimateRotationToInterfaceOrientation:duration:. The latter is called
// during an "animation block", and its unclear that reloading a collection view
// will always work during an "animation block", although it appears to work at
// first glance.
// TODO(crbug.com/705339): Replace this deprecated method with
// viewWillTransitionToSize:withTransitionCoordinator:
- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)orientation
                                duration:(NSTimeInterval)duration {
  [super willRotateToInterfaceOrientation:orientation duration:duration];
  [self updateUIForInterfaceOrientation:orientation duration:duration];
}

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  UIInterfaceOrientation orient = GetInterfaceOrientation();
  [self updateUIForInterfaceOrientation:orient duration:0];
}

- (BOOL)prefersStatusBarHidden {
  return NO;
}

- (NSArray*)keyCommands {
  __weak BookmarkHomeHandsetViewController* weakSelf = self;
  return @[ [UIKeyCommand cr_keyCommandWithInput:UIKeyInputEscape
                                   modifierFlags:Cr_UIKeyModifierNone
                                           title:nil
                                          action:^{
                                            [weakSelf navigationBarCancel:nil];
                                          }] ];
}

#pragma mark - Superclass overrides

- (void)loadBookmarkViews {
  [super loadBookmarkViews];
  DCHECK(self.bookmarks->loaded());
  DCHECK([self isViewLoaded]);
}

- (void)updatePrimaryMenuItem:(BookmarkMenuItem*)menuItem
                     animated:(BOOL)animated {
  [super updatePrimaryMenuItem:menuItem animated:animated];
}

- (ActionSheetCoordinator*)createActionSheetCoordinatorOnView:(UIView*)view {
  return [[ActionSheetCoordinator alloc] initWithBaseViewController:self
                                                              title:nil
                                                            message:nil
                                                               rect:CGRectZero
                                                               view:nil];
}

#pragma mark - BookmarkMenuViewDelegate

- (void)bookmarkMenuView:(BookmarkMenuView*)view
        selectedMenuItem:(BookmarkMenuItem*)menuItem {
  BOOL menuItemChanged = ![[self primaryMenuItem] isEqual:menuItem];
  // If the primary menu item has changed, then updatePrimaryMenuItem: will
  // update the navigation bar, and there's no need for hideMenuAnimated: to do
  // so.
  if (menuItemChanged)
    [self updatePrimaryMenuItem:menuItem animated:YES];
}

#pragma mark - Private.

- (CGRect)frameForPanelView {
  return self.view.frame;
}

- (void)updateUIForInterfaceOrientation:(UIInterfaceOrientation)orientation
                               duration:(NSTimeInterval)duration {
}

- (void)showMenuAnimated:(BOOL)animated {
  [self.menuView setScrollsToTop:YES];
  self.scrollToTop = YES;
  [self.panelView showMenuAnimated:animated];
}

- (void)hideMenuAnimated:(BOOL)animated updateNavigationBar:(BOOL)update {
  [self.menuView setScrollsToTop:NO];
  self.scrollToTop = NO;
  [self.panelView hideMenuAnimated:animated];
}

#pragma mark - BookmarkPanelViewDelegate

- (void)bookmarkPanelView:(BookmarkPanelView*)view
             willShowMenu:(BOOL)showMenu
    withAnimationDuration:(CGFloat)duration {
  if (showMenu) {
    [self.menuView setScrollsToTop:YES];
    self.scrollToTop = YES;
  } else {
    [self.menuView setScrollsToTop:NO];
    self.scrollToTop = NO;
  }
}

- (void)bookmarkPanelView:(BookmarkPanelView*)view
    updatedMenuVisibility:(CGFloat)visibility {
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return self.editing ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

@end

