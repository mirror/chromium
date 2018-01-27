// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_header_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_toolbar_controller.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_utils.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_snapshot_providing.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ContentSuggestionsHeaderView ()<ToolbarSnapshotProviding> {
  UIView* _toolbarView;
}

@end

@implementation ContentSuggestionsHeaderView

#pragma mark - Public

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.clipsToBounds = YES;
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _toolbarView.hidden = !IsLandscape();
}

- (UIView*)toolBarView {
  return _toolbarView;
}

- (void)addToolbarView:(UIView*)toolbarView {
  DCHECK(!_toolbarView);
  _toolbarView = toolbarView;
  [self addSubview:_toolbarView];
  [NSLayoutConstraint activateConstraints:@[
    [_toolbarView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [_toolbarView.topAnchor constraintEqualToAnchor:self.topAnchor],
    [_toolbarView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
  ]];
}

- (void)addViewsToSearchField:(UIView*)searchField {
  // TODO(crbug.com/805644) Update fake omnibox theme.
}

- (void)updateSearchFieldWidth:(NSLayoutConstraint*)widthConstraint
                        height:(NSLayoutConstraint*)heightConstraint
                     topMargin:(NSLayoutConstraint*)topMarginConstraint
            subviewConstraints:(NSArray*)constraints
                     forOffset:(CGFloat)offset
                   screenWidth:(CGFloat)screenWidth
                safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  CGFloat contentWidth = std::max<CGFloat>(
      0, screenWidth - safeAreaInsets.left - safeAreaInsets.right);
  // The scroll offset at which point searchField's frame should stop growing.
  CGFloat maxScaleOffset =
      self.frame.size.height - ntp_header::kMinHeaderHeight;
  // The scroll offset at which point searchField's frame should start
  // growing.
  CGFloat startScaleOffset = maxScaleOffset - ntp_header::kAnimationDistance;
  CGFloat percent = 0;
  if (offset > startScaleOffset) {
    CGFloat animatingOffset = offset - startScaleOffset;
    percent = MIN(1, MAX(0, animatingOffset / ntp_header::kAnimationDistance));
  }

  if (screenWidth == 0 || contentWidth == 0)
    return;

  CGFloat searchFieldNormalWidth =
      content_suggestions::searchFieldWidth(contentWidth);

  // Calculate the amount to grow the width and height of searchField so that
  // its frame covers the entire toolbar area.
  CGFloat maxXInset = ui::AlignValueToUpperPixel(
      (searchFieldNormalWidth - screenWidth) / 2 - 1);
  CGFloat maxHeightDiff =
      ntp_header::kToolbarHeight - content_suggestions::kSearchFieldHeight;

  widthConstraint.constant = searchFieldNormalWidth - 2 * maxXInset * percent;
  topMarginConstraint.constant = -content_suggestions::searchFieldTopMargin() -
                                 ntp_header::kMaxTopMarginDiff * percent;
  heightConstraint.constant =
      content_suggestions::kSearchFieldHeight + maxHeightDiff * percent;

  // Adjust the position of the search field's subviews by adjusting their
  // constraint constant value.
  CGFloat constantDiff =
      percent * (ntp_header::kMaxHorizontalMarginDiff + safeAreaInsets.left);
  for (NSLayoutConstraint* constraint in constraints) {
    if (constraint.constant > 0)
      constraint.constant = constantDiff + ntp_header::kHintLabelSidePadding;
    else
      constraint.constant = -constantDiff;
  }
}

#pragma mark - ToolbarOwner

- (CGRect)toolbarFrame {
  return _toolbarView.frame;
}

- (id<ToolbarSnapshotProviding>)toolbarSnapshotProvider {
  return self;
}

#pragma mark - ToolbarSnapshotProviding

- (UIView*)snapshotForTabSwitcher {
  return nil;
}

- (UIView*)snapshotForStackViewWithWidth:(CGFloat)width
                          safeAreaInsets:(UIEdgeInsets)safeAreaInsets {
  return _toolbarView;
}

- (UIColor*)toolbarBackgroundColor {
  return [UIColor whiteColor];
}

// NoOp
- (void)fadeOutShadow {
}

- (void)hideToolbarViewsForNewTabPage {
}

- (void)setToolbarTabCount:(int)tabCount {
}

- (void)setCanGoForward:(BOOL)canGoForward {
}
- (void)setCanGoBack:(BOOL)canGoBack {
}
@end
