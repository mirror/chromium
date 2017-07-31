// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/content_widget_extension/most_visited_tile_view.h"

#import "ios/chrome/browser/ui/constraints_ui_util.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kLabelTextColor = 0.314;
const NSInteger kLabelNumLines = 2;
const CGFloat kFaviconSize = 48;
const CGFloat kSpaceFaviconTitle = 8;

// Width of a tile.
const CGFloat kTileWidth = 73;
}

@implementation MostVisitedTileView

@synthesize faviconView = _faviconView;
@synthesize titleLabel = _titleLabel;

#pragma mark - Public

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _titleLabel.textColor = [UIColor colorWithWhite:kLabelTextColor alpha:1.0];
    _titleLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    _titleLabel.textAlignment = NSTextAlignmentCenter;
    _titleLabel.isAccessibilityElement = NO;
    _titleLabel.numberOfLines = kLabelNumLines;

    _faviconView = [[FaviconViewNew alloc] init];
    _faviconView.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];

    UIStackView* stack = [[UIStackView alloc]
        initWithArrangedSubviews:@[ _faviconView, _titleLabel ]];
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = kSpaceFaviconTitle;
    stack.alignment = UIStackViewAlignmentCenter;
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:stack];
    AddSameConstraints(self, stack);

    // A transparent button constrained to the same size as the stack is added
    // to handle taps on the stack view.
    UIButton* actionButton = [[UIButton alloc] initWithFrame:CGRectZero];
    actionButton.backgroundColor = [UIColor clearColor];
    //    [actionButton addTarget:target
    //                     action:actionSelector
    //           forControlEvents:UIControlEventTouchUpInside];
    actionButton.translatesAutoresizingMaskIntoConstraints = NO;
    actionButton.accessibilityLabel = @"LABEL TO DO CHANGE";
    [self addSubview:actionButton];
    AddSameConstraints(stack, actionButton);

    [NSLayoutConstraint activateConstraints:@[
      [stack.widthAnchor constraintEqualToConstant:kTileWidth],
      [_faviconView.widthAnchor constraintEqualToConstant:kFaviconSize],
      [_faviconView.heightAnchor constraintEqualToConstant:kFaviconSize],
    ]];

    self.isAccessibilityElement = YES;
  }
  return self;
}

@end
