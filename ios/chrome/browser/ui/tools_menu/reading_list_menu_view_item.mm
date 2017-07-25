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

// Accessibility identifier for the text badge on the reading list cell.
NSString* const kReadingListTextBadgeAccessibilityIdentifier =
    @"kReadingListTextBadgeAccessibilityIdentifier";

namespace {
// ID for cell reuse
static NSString* const kReadingListCellID = @"ReadingListCellID";
const CGFloat kToolsMenuItemTrailingMargin = 25;
// How long (in seconds) it takes to animate in / out the text badge.
const NSTimeInterval kTextBadgeAnimationDuration = ios::material::kDuration3;
}  // namespace

@interface ReadingListMenuViewCell () {
  // A badge displaying how many unread items are in the user's reading list.
  // Only one of |_numberBadge| and |_textBadge| can be shown at once. If both
  // try to be shown at once, |_numberBadge| takes precedence over |_textBadge|.
  NumberBadgeView* _numberBadge;
  // A badge displaying "NEW" to draw attention to the reading list cell.
  TextBadgeView* _textBadge;
}

// Whether or not the number badge is hidden. Defaults to |YES|.
@property(nonatomic, assign, getter=isNumberBadgeHidden) BOOL numberBadgeHidden;
// Whether or not the text badge is hidden. Defaults to |YES|.
@property(nonatomic, assign, getter=isTextBadgeHidden) BOOL textBadgeHidden;

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

@synthesize numberBadgeHidden = _numberBadgeHidden;
@synthesize textBadgeHidden = _textBadgeHidden;

- (void)initializeViews {
  if (_numberBadge && _textBadge && [self title]) {
    return;
  }

  [super initializeViews];

  self.numberBadgeHidden = YES;
  _numberBadge = [[NumberBadgeView alloc] initWithFrame:CGRectZero];
  [_numberBadge setTranslatesAutoresizingMaskIntoConstraints:NO];
  [self.contentView addSubview:_numberBadge];

  [self.contentView removeConstraints:self.contentView.constraints];

  [NSLayoutConstraint activateConstraints:@[
    [self.title.centerYAnchor
        constraintEqualToAnchor:self.contentView.centerYAnchor]
  ]];
  [self addConstraintsToBadge:_numberBadge];

  self.textBadgeHidden = YES;
  NSString* text = l10n_util::GetNSStringWithFixup(
      IDS_IOS_READING_LIST_CELL_NEW_FEATURE_BADGE);
  NSString* uppercaseText =
      [text uppercaseStringWithLocale:[NSLocale currentLocale]];
  _textBadge = [[TextBadgeView alloc] initWithText:uppercaseText];
  [_textBadge setTranslatesAutoresizingMaskIntoConstraints:NO];
  _textBadge.accessibilityIdentifier =
      kReadingListTextBadgeAccessibilityIdentifier;
  _textBadge.accessibilityLabel = text;
  [self.contentView addSubview:_textBadge];

  [self addConstraintsToBadge:_textBadge];
}

- (void)updateBadgeCount:(NSInteger)count animated:(BOOL)animated {
  [_numberBadge setNumber:count animated:animated];
  self.numberBadgeHidden = (count > 0 ? NO : YES);
  // If the number badge is shown, then the text badge must be hidden.
  if (!self.isNumberBadgeHidden && !self.isTextBadgeHidden) {
    [self updateShowTextBadge:NO animated:animated];
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

- (void)updateShowTextBadge:(BOOL)showTextBadge animated:(BOOL)animated {
  // Only 1 badge can be visible at a time, and the number badge takes priority.
  if (showTextBadge && !self.isNumberBadgeHidden) {
    return;
  }
  self.textBadgeHidden = !showTextBadge;
  CGFloat alpha = (showTextBadge ? 1.0 : 0.0);
  NSTimeInterval duration = (animated ? kTextBadgeAnimationDuration : 0.0);
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

@end
