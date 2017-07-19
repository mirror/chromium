// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/reading_list_menu_view_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/reading_list/number_badge_view.h"
#import "ios/chrome/browser/ui/reading_list/text_badge_view.h"
#import "ios/chrome/common/material_timing.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kReadingListNewFeatureBadgeAccessibilityIdentifier =
    @"kReadingListNewFeatureBadgeAccessibilityIdentifier";

namespace {
// ID for cell reuse
static NSString* const kReadingListCellID = @"ReadingListCellID";
const CGFloat kToolsMenuItemTrailingMargin = 25;
const NSTimeInterval kAnimationDuration = ios::material::kDuration3;
}  // namespace

@interface ReadingListMenuViewCell () {
  NumberBadgeView* _badge;
  TextBadgeView* _newBadge;
}

- (void)addConstraintsToBadge:(UIView*)badgeView;
+ (void)addConstraintsToBadge:(UIView*)badgeView
                     andTitle:(UIView*)title
            withLeadingMargin:(CGFloat)leadingMargin
           withTrailingMargin:(CGFloat)trailingMargin
                       inView:(UIView*)containerView;

@end

@implementation ReadingListMenuViewItem

+ (NSString*)cellID {
  return kReadingListCellID;
}

+ (Class)cellClass {
  return [ReadingListMenuViewCell class];
}

@end

@implementation ReadingListMenuViewCell

- (void)initializeViews {
  if (_badge && _newBadge && [self title]) {
    return;
  }

  [super initializeViews];

  _badge = [[NumberBadgeView alloc] initWithFrame:CGRectZero];
  [_badge setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self.contentView addSubview:_badge];

  [self.contentView removeConstraints:self.contentView.constraints];

  [NSLayoutConstraint activateConstraints:@[
    [self.title.centerYAnchor
        constraintEqualToAnchor:self.contentView.centerYAnchor]
  ]];
  [self addConstraintsToBadge:_badge];

  NSString* text = l10n_util::GetNSStringWithFixup(
      IDS_IOS_READING_LIST_CELL_NEW_FEATURE_BADGE);
  _newBadge = [[TextBadgeView alloc] initWithText:text];
  [_newBadge setTranslatesAutoresizingMaskIntoConstraints:NO];
  _newBadge.accessibilityIdentifier =
      kReadingListNewFeatureBadgeAccessibilityIdentifier;
  _newBadge.accessibilityLabel = text;
  [self.contentView addSubview:_newBadge];

  [self addConstraintsToBadge:_newBadge];
}

- (void)addConstraintsToBadge:(UIView*)badgeView {
  [ReadingListMenuViewCell addConstraintsToBadge:badgeView
                                        andTitle:self.title
                               withLeadingMargin:self.horizontalMargin
                              withTrailingMargin:kToolsMenuItemTrailingMargin
                                          inView:self.contentView];
}

+ (void)addConstraintsToBadge:(UIView*)badgeView
                     andTitle:(UIView*)titleView
            withLeadingMargin:(CGFloat)leadingMargin
           withTrailingMargin:(CGFloat)trailingMargin
                       inView:(UIView*)containerView {
  NSMutableArray<NSLayoutConstraint*>* constraintsToApply = [NSMutableArray
      arrayWithArray:
          [NSLayoutConstraint
              constraintsWithVisualFormat:
                  @"H:|-(margin)-[title]-[badge]-(endMargin)-|"
                                  options:
                                      NSLayoutFormatDirectionLeadingToTrailing
                                  metrics:@{
                                    @"margin" : @(leadingMargin),
                                    @"endMargin" : @(trailingMargin)
                                  }
                                    views:@{
                                      @"title" : titleView,
                                      @"badge" : badgeView
                                    }]];

  [constraintsToApply
      addObject:[badgeView.centerYAnchor
                    constraintEqualToAnchor:containerView.centerYAnchor]];

  [NSLayoutConstraint activateConstraints:constraintsToApply];
}

- (void)updateBadgeCount:(NSInteger)count animated:(BOOL)animated {
  [_badge setNumber:count animated:animated];
}

- (void)updateSeenState:(BOOL)hasUnseenItems animated:(BOOL)animated {
  if (hasUnseenItems) {
    UIColor* highlightedColor = [[MDCPalette cr_bluePalette] tint500];

    [_badge setBackgroundColor:highlightedColor animated:animated];
    [self.title setTextColor:highlightedColor];
  } else {
    UIColor* regularColor = [[MDCPalette greyPalette] tint500];

    [_badge setBackgroundColor:regularColor animated:animated];
    [self.title setTextColor:[UIColor blackColor]];
  }
}

- (void)updateShowNewFeatureBadge:(BOOL)showNewFeatureBadge
                         animated:(BOOL)animated {
  CGFloat alpha = (showNewFeatureBadge ? 1.0 : 0.0);
  if (animated) {
    [UIView animateWithDuration:kAnimationDuration
                     animations:^{
                       _newBadge.alpha = alpha;
                     }];
  } else {
    _newBadge.alpha = alpha;
  }
}

@end
