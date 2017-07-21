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
const NSTimeInterval kNewFeatureBadgeAnimationDuration =
    ios::material::kDuration3;
}  // namespace

@interface ReadingListMenuViewCell () {
  // Whether or not |_numberBadge| is visible.
  BOOL _numberBadgeIsVisible;
  // Whether or not |_textBadge| is visible.
  BOOL _textBadgeIsVisible;
  // A badge displaying how many unread items are in the user's reading list.
  // Only one of |_numberBadge| and |_textBadge| can be shown at once. If both
  // try to be shown at once, |_numberBadge| takes precedence over |_textBadge|.
  NumberBadgeView* _numberBadge;
  // A badge displaying "NEW" to draw attention to the reading list cell.
  TextBadgeView* _textBadge;
}

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
  if (_numberBadge && _textBadge && [self title]) {
    return;
  }

  [super initializeViews];

  _numberBadge = [[NumberBadgeView alloc] initWithFrame:CGRectZero];
  [_numberBadge setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self.contentView addSubview:_numberBadge];

  [self.contentView removeConstraints:self.contentView.constraints];

  [NSLayoutConstraint activateConstraints:@[
    [self.title.centerYAnchor
        constraintEqualToAnchor:self.contentView.centerYAnchor]
  ]];
  [self addConstraintsToBadge:_numberBadge];

  NSString* text = l10n_util::GetNSStringWithFixup(
      IDS_IOS_READING_LIST_CELL_NEW_FEATURE_BADGE);
  _textBadge = [[TextBadgeView alloc] initWithText:text];
  [_textBadge setTranslatesAutoresizingMaskIntoConstraints:NO];
  _textBadge.accessibilityIdentifier =
      kReadingListNewFeatureBadgeAccessibilityIdentifier;
  _textBadge.accessibilityLabel = text;
  [self.contentView addSubview:_textBadge];

  [self addConstraintsToBadge:_textBadge];
}

- (void)updateBadgeCount:(NSInteger)count animated:(BOOL)animated {
  [_numberBadge setNumber:count animated:animated];
  _numberBadgeIsVisible = (count > 0 ? YES : NO);
  // If the number badge is shown, then the text badge must be hidden.
  if (_numberBadgeIsVisible && _textBadgeIsVisible) {
    [self updateShowNewFeatureBadge:NO animated:animated];
  }
}

- (void)updateSeenState:(BOOL)hasUnseenItems animated:(BOOL)animated {
  if (hasUnseenItems) {
    UIColor* highlightedColor = [[MDCPalette cr_bluePalette] tint500];

    [_numberBadge setBackgroundColor:highlightedColor animated:animated];
    [self.title setTextColor:highlightedColor];
  } else {
    UIColor* regularColor = [[MDCPalette greyPalette] tint500];

    [_numberBadge setBackgroundColor:regularColor animated:animated];
    [self.title setTextColor:[UIColor blackColor]];
  }
}

- (void)updateShowNewFeatureBadge:(BOOL)showNewFeatureBadge
                         animated:(BOOL)animated {
  // Only 1 badge can be visible at a time, and the number badge takes priority.
  if (showNewFeatureBadge && _numberBadgeIsVisible) {
    return;
  }
  _textBadgeIsVisible = showNewFeatureBadge;
  CGFloat alpha = (showNewFeatureBadge ? 1.0 : 0.0);
  NSTimeInterval duration =
      (animated ? kNewFeatureBadgeAnimationDuration : 0.0);
  // If |duration| is 0.0, the block is executed immediately (that is, the
  // properties are set directly without any actual animation). Else, the
  // animation occurs as normal.
  [UIView animateWithDuration:duration
                   animations:^{
                     _textBadge.alpha = alpha;
                   }];
}

#pragma mark - Private Methods

// Adds autolayout constraints to |badgeView| that center it vertically in the
// cell and anchor it to the right.
- (void)addConstraintsToBadge:(UIView*)badgeView {
  NSMutableArray<NSLayoutConstraint*>* constraintsToApply = [NSMutableArray
      arrayWithArray:
          [NSLayoutConstraint
              constraintsWithVisualFormat:
                  @"H:|-(margin)-[title]-[badge]-(endMargin)-|"
                                  options:
                                      NSLayoutFormatDirectionLeadingToTrailing
                                  metrics:@{
                                    @"margin" : @(self.horizontalMargin),
                                    @"endMargin" :
                                        @(kToolsMenuItemTrailingMargin)
                                  }
                                    views:@{
                                      @"title" : self.title,
                                      @"badge" : badgeView
                                    }]];

  [constraintsToApply
      addObject:[badgeView.centerYAnchor
                    constraintEqualToAnchor:self.contentView.centerYAnchor]];

  [NSLayoutConstraint activateConstraints:constraintsToApply];
}

- (BOOL)numberBadgeIsVisible {
  return _numberBadgeIsVisible;
}

- (BOOL)textBadgeIsVisible {
  return _textBadgeIsVisible;
}

@end
