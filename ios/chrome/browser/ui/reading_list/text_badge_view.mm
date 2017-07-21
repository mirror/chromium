// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/text_badge_view.h"

#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat labelMargin = 2.5f;
}

@interface TextBadgeView ()
@property(nonatomic, readonly, weak) UILabel* label;
@end

@implementation TextBadgeView

@synthesize text = _text;
@synthesize label = _label;

- (instancetype)initWithText:(NSString*)text {
  self = [super initWithFrame:CGRectZero];
  _text = text;
  if (self) {
    [self setTranslatesAutoresizingMaskIntoConstraints:NO];

    UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
    _label = label;
    [label setFont:[[MDCTypography fontLoader] boldFontOfSize:10]];
    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [label setTextColor:[UIColor whiteColor]];
    [label setText:text];
    [self addSubview:label];

    [NSLayoutConstraint activateConstraints:@[
      // Center label on badge.
      [label.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
      [label.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],

      // Make badge fit label.
      [self.heightAnchor constraintEqualToAnchor:label.heightAnchor
                                        constant:labelMargin * 2],
      [self.widthAnchor constraintGreaterThanOrEqualToAnchor:self.heightAnchor],
      [self.widthAnchor constraintGreaterThanOrEqualToAnchor:label.widthAnchor
                                                    constant:labelMargin * 4]
    ]];

    [self setBackgroundColor:[[MDCPalette cr_bluePalette] tint500]];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  self.layer.cornerRadius = self.bounds.size.height / 2.0f;
}

- (void)setText:(NSString*)text {
  if (self.text != text) {
    _text = text;
    self.label.text = text;
  }
}

@end
