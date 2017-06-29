// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/search_widget_extension/search_widget_view.h"
#include "base/ios/ios_util.h"
#include "base/logging.h"
#import "ios/chrome/search_widget_extension/copied_url_view.h"
#import "ios/chrome/search_widget_extension/search_action_view.h"
#import "ios/chrome/search_widget_extension/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kMaxContentSize = 421;

}  // namespace

@interface SearchWidgetView ()

// The target for actions in the view.
@property(nonatomic, weak) id<SearchWidgetViewActionTarget> target;
// The primary effect of the widget. Use for a more opaque appearance.
@property(nonatomic, strong) UIVisualEffect* primaryEffect;
// The secondary effect of the widget. Use for a more transparent appearance.
@property(nonatomic, strong) UIVisualEffect* secondaryEffect;
// The copied URL view.
@property(nonatomic, strong) CopiedURLView* copiedURLView;

// Sets up the widget UI.
- (void)createUI;

// Creates the view for the action buttons.
- (UIView*)newActionsView;

@end

@implementation SearchWidgetView

@synthesize target = _target;
@synthesize primaryEffect = _primaryEffect;
@synthesize secondaryEffect = _secondaryEffect;
@synthesize copiedURLView = _copiedURLView;

- (instancetype)initWithActionTarget:(id<SearchWidgetViewActionTarget>)target
               primaryVibrancyEffect:(UIVibrancyEffect*)primaryVibrancyEffect
             secondaryVibrancyEffect:
                 (UIVibrancyEffect*)secondaryVibrancyEffect {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK(target);
    _target = target;
    _primaryEffect = primaryVibrancyEffect;
    _secondaryEffect = secondaryVibrancyEffect;
    [self createUI];
  }
  return self;
}

#pragma mark - UI creation

- (void)createUI {
  UIView* actionsView = [self newActionsView];
  [self addSubview:actionsView];

  _copiedURLView =
      [[CopiedURLView alloc] initWithActionTarget:self.target
                                   actionSelector:@selector(openCopiedURL:)
                                    primaryEffect:self.primaryEffect
                                  secondaryEffect:self.secondaryEffect];
  [self addSubview:_copiedURLView];

  // These constraints stretch the action row to the full width of the widget.
  // Their priority is < UILayoutPriorityRequired so that they can break when
  // the view is larger than kMaxContentSize.
  NSLayoutConstraint* actionsLeftConstraint =
      [actionsView.leftAnchor constraintEqualToAnchor:self.leftAnchor];
  actionsLeftConstraint.priority = UILayoutPriorityDefaultHigh;

  NSLayoutConstraint* actionsRightConstraint =
      [actionsView.rightAnchor constraintEqualToAnchor:self.rightAnchor];
  actionsRightConstraint.priority = UILayoutPriorityDefaultHigh;

  [NSLayoutConstraint activateConstraints:@[
    [actionsView.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
    [actionsView.widthAnchor
        constraintLessThanOrEqualToConstant:kMaxContentSize],
    actionsLeftConstraint,
    actionsRightConstraint,
    [actionsView.topAnchor constraintEqualToAnchor:self.topAnchor
                                          constant:ui_util::kContentMargin],
    [actionsView.bottomAnchor constraintEqualToAnchor:_copiedURLView.topAnchor],
    [_copiedURLView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [_copiedURLView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [_copiedURLView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
  ]];
}

- (UIView*)newActionsView {
  UIStackView* actionRow = [[UIStackView alloc] initWithArrangedSubviews:@[
    [[SearchActionView alloc]
        initWithActionTarget:self.target
              actionSelector:@selector(openSearch:)
               primaryEffect:self.primaryEffect
             secondaryEffect:self.secondaryEffect
                       title:NSLocalizedString(@"IDS_IOS_NEW_SEARCH",
                                               @"New Search")
                   imageName:@"quick_action_search"],

    [[SearchActionView alloc]
        initWithActionTarget:self.target
              actionSelector:@selector(openSearch:)
               primaryEffect:self.primaryEffect
             secondaryEffect:self.secondaryEffect
                       title:NSLocalizedString(@"IDS_IOS_INCOGNITO_SEARCH",
                                               @"Incognito Search")
                   imageName:@"quick_action_incognito_search"],
    [[SearchActionView alloc]
        initWithActionTarget:self.target
              actionSelector:@selector(openSearch:)
               primaryEffect:self.primaryEffect
             secondaryEffect:self.secondaryEffect
                       title:NSLocalizedString(@"IDS_IOS_VOICE_SEARCH",
                                               @"Voice Search")
                   imageName:@"quick_action_voice_search"],
    [[SearchActionView alloc]
        initWithActionTarget:self.target
              actionSelector:@selector(openSearch:)
               primaryEffect:self.primaryEffect
             secondaryEffect:self.secondaryEffect
                       title:NSLocalizedString(@"IDS_IOS_SCAN_QR_CODE",
                                               @"Scan QR Code")
                   imageName:@"quick_action_camera_search"],
  ]];

  actionRow.axis = UILayoutConstraintAxisHorizontal;
  actionRow.alignment = UIStackViewAlignmentTop;
  actionRow.distribution = UIStackViewDistributionFillEqually;
  actionRow.spacing = ui_util::kIconSpacing;
  actionRow.layoutMargins =
      UIEdgeInsetsMake(0, ui_util::kContentMargin, 0, ui_util::kContentMargin);
  actionRow.layoutMarginsRelativeArrangement = YES;
  actionRow.translatesAutoresizingMaskIntoConstraints = NO;
  return actionRow;
}

- (void)setCopiedURLString:(NSString*)URL {
  [self.copiedURLView setCopiedURLString:URL];
}

@end
