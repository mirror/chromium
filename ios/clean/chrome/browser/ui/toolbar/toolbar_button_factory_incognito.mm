// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_button_factory_incognito.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/grit/ios_theme_resources.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_button.h"
#import "ios/clean/chrome/browser/ui/toolbar/toolbar_constants.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ToolbarButtonFactoryIncognito

- (ToolbarButton*)backToolbarButton {
  ToolbarButton* backButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeReversableImage(
                                               IDR_IOS_TOOLBAR_DARK_BACK, YES)
                  imageForHighlightedState:
                      NativeReversableImage(IDR_IOS_TOOLBAR_DARK_BACK_PRESSED,
                                            YES)
                     imageForDisabledState:
                         NativeReversableImage(
                             IDR_IOS_TOOLBAR_DARK_BACK_DISABLED, YES)];
  backButton.accessibilityLabel = l10n_util::GetNSString(IDS_ACCNAME_BACK);
  return backButton;
}

- (ToolbarButton*)forwardToolbarButton {
  ToolbarButton* forwardButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeReversableImage(
                                               IDR_IOS_TOOLBAR_DARK_FORWARD,
                                               YES)
                  imageForHighlightedState:
                      NativeReversableImage(
                          IDR_IOS_TOOLBAR_DARK_FORWARD_PRESSED, YES)
                     imageForDisabledState:
                         NativeReversableImage(
                             IDR_IOS_TOOLBAR_DARK_FORWARD_DISABLED, YES)];
  forwardButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_ACCNAME_FORWARD);
  return forwardButton;
}

- (ToolbarButton*)tabSwitcherStripToolbarButton {
  ToolbarButton* tabSwitcherStripButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeImage(
                                               IDR_IOS_TOOLBAR_DARK_OVERVIEW)
                  imageForHighlightedState:
                      NativeImage(IDR_IOS_TOOLBAR_DARK_OVERVIEW_PRESSED)
                     imageForDisabledState:
                         NativeImage(IDR_IOS_TOOLBAR_DARK_OVERVIEW_DISABLED)];
  tabSwitcherStripButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_SHOW_TABS);
  [tabSwitcherStripButton
      setTitleColor:UIColorFromRGB(kIncognitoToolbarButtonTitleNormalColor)
           forState:UIControlStateNormal];
  [tabSwitcherStripButton
      setTitleColor:UIColorFromRGB(kIncognitoToolbarButtonTitleHighlightedColor)
           forState:UIControlStateHighlighted];
  return tabSwitcherStripButton;
}

- (ToolbarButton*)tabSwitcherGridToolbarButton {
  ToolbarButton* tabSwitcherGridButton =
      [ToolbarButton toolbarButtonWithImageForNormalState:
                         [UIImage imageNamed:@"tabswitcher_tab_switcher_button"]
                                 imageForHighlightedState:nil
                                    imageForDisabledState:nil];
  tabSwitcherGridButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_SHOW_TAB_GRID);
  return tabSwitcherGridButton;
}

- (ToolbarButton*)toolsMenuToolbarButton {
  ToolbarButton* toolsMenuButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeImage(
                                               IDR_IOS_TOOLBAR_DARK_TOOLS)
                  imageForHighlightedState:
                      NativeImage(IDR_IOS_TOOLBAR_DARK_TOOLS_PRESSED)
                     imageForDisabledState:nil];
  [toolsMenuButton setImageEdgeInsets:UIEdgeInsetsMakeDirected(0, -3, 0, 0)];
  toolsMenuButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_TOOLBAR_SETTINGS);
  return toolsMenuButton;
}

- (ToolbarButton*)shareToolbarButton {
  ToolbarButton* shareButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeImage(
                                               IDR_IOS_TOOLBAR_DARK_SHARE)
                  imageForHighlightedState:
                      NativeImage(IDR_IOS_TOOLBAR_DARK_SHARE_PRESSED)
                     imageForDisabledState:
                         NativeImage(IDR_IOS_TOOLBAR_DARK_SHARE_DISABLED)];
  shareButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_TOOLS_MENU_SHARE);
  return shareButton;
}

- (ToolbarButton*)reloadToolbarButton {
  ToolbarButton* reloadButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeReversableImage(
                                               IDR_IOS_TOOLBAR_DARK_RELOAD, YES)
                  imageForHighlightedState:
                      NativeReversableImage(IDR_IOS_TOOLBAR_DARK_RELOAD_PRESSED,
                                            YES)
                     imageForDisabledState:
                         NativeReversableImage(
                             IDR_IOS_TOOLBAR_DARK_RELOAD_DISABLED, YES)];
  reloadButton.accessibilityLabel =
      l10n_util::GetNSString(IDS_IOS_ACCNAME_RELOAD);
  return reloadButton;
}

- (ToolbarButton*)stopToolbarButton {
  ToolbarButton* stopButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeImage(
                                               IDR_IOS_TOOLBAR_DARK_STOP)
                  imageForHighlightedState:
                      NativeImage(IDR_IOS_TOOLBAR_DARK_STOP_PRESSED)
                     imageForDisabledState:
                         NativeImage(IDR_IOS_TOOLBAR_DARK_STOP_DISABLED)];
  stopButton.accessibilityLabel = l10n_util::GetNSString(IDS_IOS_ACCNAME_STOP);
  return stopButton;
}

- (ToolbarButton*)starToolbarButton {
  ToolbarButton* starButton = [ToolbarButton
      toolbarButtonWithImageForNormalState:NativeImage(
                                               IDR_IOS_TOOLBAR_DARK_STAR)
                  imageForHighlightedState:
                      NativeImage(IDR_IOS_TOOLBAR_DARK_STAR_PRESSED)
                     imageForDisabledState:nil];
  starButton.accessibilityLabel = l10n_util::GetNSString(IDS_TOOLTIP_STAR);
  return starButton;
}

@end
