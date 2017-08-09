// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/text_badge_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kFontSize = 10.0f;
// The margin between the top and bottom of the label and the badge.
const CGFloat kLabelVerticalMargin = 2.5f;
// The identifier for the badge width constraint that relies on the label's
// horizontal margin.
NSString* kWidthLabelMarginConstraintIdentifier = @"WidthLabelMarginConstraint";
}

@interface TextBadgeView ()
@property(nonatomic, readonly, strong) UILabel* label;
// Indicate whether |label| needs to be added as a subview of the TextBadgeView.
@property(nonatomic, assign) BOOL needsAddSubviews;
@end

@implementation TextBadgeView

@synthesize label = _label;
@synthesize labelHorizontalMargin = _labelHorizontalMargin;
@synthesize needsAddSubviews = _needsAddSubviews;

- (instancetype)initWithText:(NSString*)text {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    UILabel* label = [TextBadgeView labelWithText:text];
    _label = label;
    // Initialize |labelHorizontalMargin| to the default value.
    _labelHorizontalMargin = 8.5f;
    _needsAddSubviews = YES;
  }
  return self;
}

#pragma mark - UIView overrides

// Override |willMoveToSuperview| to add view properties to the view hierarchy
// and set the badge's appearance.
- (void)willMoveToSuperview:(UIView*)newSuperview {
  // Only add subviews if needed.
  if (self.needsAddSubviews) {
    [self addSubview:self.label];
    // Set |needsAddSubviews| to NO to ensure that subviews are only added once.
    self.needsAddSubviews = NO;
    // Perform additional setup operations, such as activating constraints,
    // setting the background color, and setting the accessibility label.
    [self activateConstraints];
    [self setBackgroundColor:[[MDCPalette cr_bluePalette] tint500]];
    [self setAccessibilityLabel:self.label.text];
  }
  [super willMoveToSuperview:newSuperview];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  // Set the badge's corner radius to be one half of its height. This causes the
  // ends of the badge to be circular.
  self.layer.cornerRadius = self.bounds.size.height / 2.0f;
}

#pragma mark - Public properties

- (NSString*)text {
  return self.label.text;
}

- (void)setText:(NSString*)text {
  DCHECK(text.length);
  self.label.text = text;
}

// Override the setter for |labelHorizontalMargin| to modify the constraint that
// depends on |labelHorizontalMargin| if necessary.
- (void)setLabelHorizontalMargin:(CGFloat)labelHorizontalMargin {
  if (labelHorizontalMargin != self.labelHorizontalMargin) {
    _labelHorizontalMargin = labelHorizontalMargin;
    // In case constraints have been added, use
    // |kWidthLabelMarginConstraintIdentifier| to find the width constraint that
    // depends on |labelHorizontalMargin|.
    for (NSLayoutConstraint* constraint in self.constraints) {
      if ([constraint.identifier
              isEqualToString:kWidthLabelMarginConstraintIdentifier]) {
        // Modify the constraint to use the new margin value.
        constraint.constant = labelHorizontalMargin * 2;
        break;
      }
    }
  }
}

#pragma mark - Private class methods

// Return a label that displays text in white with center alignment.
+ (UILabel*)labelWithText:(NSString*)text {
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  [label setFont:[[MDCTypography fontLoader] boldFontOfSize:kFontSize]];
  [label setTranslatesAutoresizingMaskIntoConstraints:NO];
  [label setTextColor:[UIColor whiteColor]];
  [label setText:text];
  [label setTextAlignment:NSTextAlignmentCenter];
  return label;
}

#pragma mark - Private instance methods

// Activate constraints to properly position the badge and its subviews.
- (void)activateConstraints {
  // Make the badge width fit the label, adding a margin on both sides.
  NSLayoutConstraint* badgeWidthConstraint =
      [self.widthAnchor constraintEqualToAnchor:self.label.widthAnchor
                                       constant:self.labelHorizontalMargin * 2];
  // This constraint should not be satisfied if the label is taller than it is
  // wide, so make it optional.
  badgeWidthConstraint.priority = UILayoutPriorityRequired - 1;
  // Since users of the text badge are able to set the label's horizontal
  // margin, give |badgeWidthConstraint| an identifier so that it can be
  // modified in |setLabelHorizontalMargin|.
  badgeWidthConstraint.identifier = kWidthLabelMarginConstraintIdentifier;

  [NSLayoutConstraint activateConstraints:@[
    // Center label on badge.
    [self.label.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [self.label.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
    // Make the badge height fit the label.
    [self.heightAnchor constraintEqualToAnchor:self.label.heightAnchor
                                      constant:kLabelVerticalMargin * 2],
    // Ensure that the badge will never be taller than it is wide. For
    // a tall label, the badge will look like a circle instead of an ellipse.
    [self.widthAnchor constraintGreaterThanOrEqualToAnchor:self.heightAnchor],
    badgeWidthConstraint
  ]];
}

@end
