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
#import "ios/chrome/browser/ui/bookmarks/bars/bookmark_editing_bar.h"
#import "ios/chrome/browser/ui/bookmarks/bars/bookmark_navigation_bar.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_collection_cells.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_collection_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_edit_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_folder_editor_view_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_primary_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller_protected.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_home_waiting_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_item.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_menu_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_navigation_controller.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_panel_view.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_position_cache.h"
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

#pragma mark Navigation bar

- (void)updateNavigationBarWithDuration:(CGFloat)duration
                            orientation:(UIInterfaceOrientation)orientation;
// Whether the edit button on the navigation bar should be shown.
- (BOOL)shouldShowEditButtonWithMenuVisibility:(BOOL)visible;
// Called when the menu button is pressed on the navigation bar.
- (void)navigationBarToggledMenu:(id)sender;

#pragma mark Private methods

// Returns the size of the panel view.
- (CGRect)frameForPanelView;
// Updates the UI to reflect the given orientation, with an animation lasting
// |duration|.
- (void)updateUIForInterfaceOrientation:(UIInterfaceOrientation)orientation
                               duration:(NSTimeInterval)duration;
// Shows or hides the menu.
- (void)showMenuAnimated:(BOOL)animated;
- (void)hideMenuAnimated:(BOOL)animated updateNavigationBar:(BOOL)update;

@end

@implementation BookmarkHomeHandsetViewController

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(ios::ChromeBrowserState*)browserState
                    dispatcher:(id<ApplicationCommands>)dispatcher {
  self = [super initWithLoader:loader
                  browserState:browserState
                    dispatcher:dispatcher];
  if (self) {
    self.sideSwipingPossible = YES;
  }
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

  // Disable editing on previous folder view before dismissing it. No need to
  // animate because this view is immediately removed from hierarchy.
  if ([[self primaryMenuItem] supportsEditing])
    [self.folderView setEditing:NO animated:NO];

  [self updateNavigationBarAnimated:animated
                        orientation:GetInterfaceOrientation()];
}

- (CGRect)editingBarFrame {
  return self.navigationBar.frame;
}

- (void)showEditingBarAnimated:(BOOL)animated {
  CGRect frame = self.navigationBar.frame;
  self.editingBar.hidden = NO;
  if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)])
    [self setNeedsStatusBarAppearanceUpdate];
  [UIView animateWithDuration:animated ? 0.2 : 0
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        self.editingBar.alpha = 1;
        self.editingBar.frame = frame;
      }
      completion:^(BOOL finished) {
        if (finished) {
          self.navigationBar.hidden = YES;
        } else {
          if ([self respondsToSelector:@selector(
                                           setNeedsStatusBarAppearanceUpdate)])
            [self setNeedsStatusBarAppearanceUpdate];
        }
      }];
}

- (void)hideEditingBarAnimated:(BOOL)animated {
  CGRect frame = self.navigationBar.frame;
  self.navigationBar.hidden = NO;
  if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)])
    [self setNeedsStatusBarAppearanceUpdate];
  [UIView animateWithDuration:animated ? 0.2 : 0
      delay:0
      options:UIViewAnimationOptionBeginFromCurrentState
      animations:^{
        self.editingBar.alpha = 0;
        self.editingBar.frame = frame;
      }
      completion:^(BOOL finished) {
        if (finished) {
          self.editingBar.hidden = YES;
        } else {
          if ([self respondsToSelector:@selector(
                                           setNeedsStatusBarAppearanceUpdate)])
            [self setNeedsStatusBarAppearanceUpdate];
        }
      }];
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
  [self hideMenuAnimated:YES updateNavigationBar:!menuItemChanged];
  if (menuItemChanged)
    [self updatePrimaryMenuItem:menuItem animated:YES];
}

#pragma mark - Navigation bar

- (CGRect)navigationBarFrame {
  return CGRectMake(0, 0, CGRectGetWidth(self.view.bounds),
                    [BookmarkNavigationBar expectedContentViewHeight] +
                        [self.topLayoutGuide length]);
}

- (void)updateNavigationBarAnimated:(BOOL)animated
                        orientation:(UIInterfaceOrientation)orientation {
  CGFloat duration = animated ? bookmark_utils_ios::menuAnimationDuration : 0;
  [self updateNavigationBarWithDuration:duration orientation:orientation];
}

- (void)updateNavigationBarWithDuration:(CGFloat)duration
                            orientation:(UIInterfaceOrientation)orientation {
  if ([self.panelView userDrivenAnimationInProgress])
    return;

  [self.navigationBar setTitle:[self.primaryMenuItem titleForNavigationBar]];
  BOOL isMenuVisible = self.panelView.showingMenu;
  if ([self shouldShowEditButtonWithMenuVisibility:isMenuVisible])
    [self.navigationBar showEditButtonWithAnimationDuration:duration];
  else
    [self.navigationBar hideEditButtonWithAnimationDuration:duration];

  if ([self shouldShowBackButtonOnNavigationBar])
    [self.navigationBar showBackButtonInsteadOfMenuButton:duration];
  else
    [self.navigationBar showMenuButtonInsteadOfBackButton:duration];
}

- (BOOL)shouldShowEditButtonWithMenuVisibility:(BOOL)visible {
  if (visible)
    return NO;
  if (self.primaryMenuItem.type != bookmarks::MenuItemFolder)
    return NO;
  // The type is MenuItemFolder, so it is safe to access |folder|.
  return !self.bookmarks->is_permanent_node(self.primaryMenuItem.folder);
}

#pragma mark Navigation Bar Callbacks

- (void)navigationBarToggledMenu:(id)sender {
  if ([self.panelView userDrivenAnimationInProgress])
    return;

  if (self.panelView.showingMenu)
    [self hideMenuAnimated:YES updateNavigationBar:YES];
  else
    [self showMenuAnimated:YES];
}

#pragma mark - Private.

- (CGRect)frameForPanelView {
  CGFloat margin;
  if (self.editing)
    margin = CGRectGetMaxY(self.editingBar.frame);
  else
    margin = CGRectGetMaxY(self.navigationBar.frame);
  CGFloat height = CGRectGetHeight(self.view.frame) - margin;
  CGFloat width = CGRectGetWidth(self.view.frame);
  return CGRectMake(0, margin, width, height);
}

- (void)updateUIForInterfaceOrientation:(UIInterfaceOrientation)orientation
                               duration:(NSTimeInterval)duration {
  [self.folderView changeOrientation:orientation];
  [self updateNavigationBarWithDuration:duration orientation:orientation];
}

- (void)showMenuAnimated:(BOOL)animated {
  [self.menuView setScrollsToTop:YES];
  [self.folderView setScrollsToTop:NO];
  self.scrollToTop = YES;
  [self.panelView showMenuAnimated:animated];
  [self updateNavigationBarAnimated:animated
                        orientation:GetInterfaceOrientation()];
}

- (void)hideMenuAnimated:(BOOL)animated updateNavigationBar:(BOOL)update {
  [self.menuView setScrollsToTop:NO];
  [self.folderView setScrollsToTop:YES];
  self.scrollToTop = NO;
  [self.panelView hideMenuAnimated:animated];
  if (update) {
    UIInterfaceOrientation orient = GetInterfaceOrientation();
    [self updateNavigationBarAnimated:animated orientation:orient];
  }
}

#pragma mark - BookmarkPanelViewDelegate

- (void)bookmarkPanelView:(BookmarkPanelView*)view
             willShowMenu:(BOOL)showMenu
    withAnimationDuration:(CGFloat)duration {
  if (showMenu) {
    [self.menuView setScrollsToTop:YES];
    [self.folderView setScrollsToTop:NO];
    self.scrollToTop = YES;
  } else {
    [self.menuView setScrollsToTop:NO];
    [self.folderView setScrollsToTop:YES];
    self.scrollToTop = NO;
  }

  if ([self shouldShowEditButtonWithMenuVisibility:showMenu])
    [self.navigationBar showEditButtonWithAnimationDuration:duration];
  else
    [self.navigationBar hideEditButtonWithAnimationDuration:duration];
}

- (void)bookmarkPanelView:(BookmarkPanelView*)view
    updatedMenuVisibility:(CGFloat)visibility {
  // As the menu becomes more visible, the edit button will always fade out.
  if ([self shouldShowEditButtonWithMenuVisibility:NO])
    [self.navigationBar updateEditButtonVisibility:1 - visibility];
  else
    [self.navigationBar updateEditButtonVisibility:0];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  return self.editing ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

@end

