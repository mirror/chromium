// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_popup_presenter.h"

#import "ios/chrome/browser/ui/omnibox/omnibox_popup_positioner.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_base_feature.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kExpandAnimationDuration = 0.1;
const CGFloat kCollapseAnimationDuration = 0.05;
const CGFloat kWhiteBackgroundHeight = 74;
NS_INLINE CGFloat ShadowHeight() {
  return IsIPadIdiom() ? 10 : 0;
}
}  // namespace

@interface OmniboxPopupPresenter ()
// Constraint for the height of the popup.
@property(nonatomic, strong) NSLayoutConstraint* heightConstraint;
// Constraint for the bottom anchor of the popup.
@property(nonatomic, strong) NSLayoutConstraint* bottomConstraint;

@property(nonatomic, weak) id<OmniboxPopupPositioner> positioner;
@property(nonatomic, weak) UITableViewController* viewController;
@property(nonatomic, strong) UIView* popupContainerView;
@end

@implementation OmniboxPopupPresenter
@synthesize viewController = _viewController;
@synthesize positioner = _positioner;
@synthesize popupContainerView = _popupContainerView;
@synthesize heightConstraint = _heightConstraint;
@synthesize bottomConstraint = _bottomConstraint;

- (instancetype)initWithPopupPositioner:(id<OmniboxPopupPositioner>)positioner
                    popupViewController:(UITableViewController*)viewController {
  self = [super init];
  if (self) {
    _positioner = positioner;
    _viewController = viewController;

    if (IsIPadIdiom()) {
      _popupContainerView = [OmniboxPopupPresenter newBackgroundViewIpad];
    } else {
      _popupContainerView = [OmniboxPopupPresenter newBackgroundViewIPhone];
    }

    CGRect popupControllerFrame = viewController.view.frame;
    popupControllerFrame.origin = CGPointZero;
    viewController.view.frame = popupControllerFrame;
    [_popupContainerView addSubview:viewController.view];
  }
  return self;
}

- (void)updateHeightAndAnimateAppearanceIfNecessary {
  UIView* popup = self.popupContainerView;
  // Show |result.size| on iPad.  Since iPhone can dismiss keyboard, set
  // height to frame height.
  CGFloat height = [[self.viewController tableView] contentSize].height;
  UIEdgeInsets insets = [[self.viewController tableView] contentInset];
  // Note the calculation |insets.top * 2| is correct, it should not be
  // insets.top + insets.bottom. |insets.bottom| will be larger than
  // |insets.top| when the keyboard is visible, but |parentHeight| should stay
  // the same.
  CGFloat parentHeight = height + insets.top * 2 + ShadowHeight();
  UIView* siblingView = [self.positioner popupAnchorView];
  BOOL newlyAdded = [popup superview] == nil;

  // Deactivate animations while adding the popup.
  [UIView setAnimationsEnabled:NO];
  if (IsIPadIdiom()) {
    [[siblingView superview] insertSubview:popup aboveSubview:siblingView];
  } else {
    [popup setAutoresizingMask:UIViewAutoresizingFlexibleWidth |
                               UIViewAutoresizingFlexibleHeight];
    [[siblingView superview] insertSubview:popup belowSubview:siblingView];
  }
  [UIView setAnimationsEnabled:YES];

  UILayoutGuide* topLayout = nil;
  for (UILayoutGuide* guide in [self.positioner popupAnchorView].layoutGuides) {
    if ([guide.identifier isEqualToString:[self.class layoutGuideIdentifier]]) {
      topLayout = guide;
      break;
    }
  }
  if (newlyAdded) {
    self.popupContainerView.translatesAutoresizingMaskIntoConstraints = NO;
    // On iPad the height of the popup is fixed.
    if (self.heightConstraint)
      self.heightConstraint.active = NO;
    self.heightConstraint =
        [self.popupContainerView.heightAnchor constraintEqualToConstant:0];
    self.heightConstraint.priority = UILayoutPriorityDefaultHigh;

    // This constraint will only be activated on iPhone as the popup is taking
    // the full height.
    self.bottomConstraint = [self.popupContainerView.bottomAnchor
        constraintEqualToAnchor:[popup superview].bottomAnchor];

    [NSLayoutConstraint activateConstraints:@[
      [self.popupContainerView.topAnchor
          constraintEqualToAnchor:topLayout.bottomAnchor],
      [self.popupContainerView.leadingAnchor
          constraintEqualToAnchor:topLayout.leadingAnchor],
      [self.popupContainerView.trailingAnchor
          constraintEqualToAnchor:topLayout.trailingAnchor],
      self.heightConstraint,
    ]];

    [self.popupContainerView layoutIfNeeded];
    [[self.popupContainerView superview] layoutIfNeeded];
  }

  CGFloat currentHeight = popup.bounds.size.height;
  if (IsIPadIdiom()) {
    self.heightConstraint.constant = parentHeight;
  } else {
    self.bottomConstraint.active = YES;
  }
  if (currentHeight == 0) {
    [self animateAppearance:parentHeight];
  }

  CGRect popupControllerFrame = self.viewController.view.frame;
  popupControllerFrame.size.height = popup.frame.size.height - ShadowHeight();
  self.viewController.view.frame = popupControllerFrame;
}

- (void)animateAppearance:(CGFloat)parentHeight {
  [UIView animateWithDuration:kExpandAnimationDuration
                        delay:0
                      options:UIViewAnimationOptionCurveEaseInOut
                   animations:^{
                     [[self.popupContainerView superview] layoutIfNeeded];
                   }
                   completion:nil];
}

- (void)animateCollapse {
  UIView* retainedPopupView = self.popupContainerView;
  self.heightConstraint.constant = 0;
  self.bottomConstraint.active = NO;
  [UIView animateWithDuration:kCollapseAnimationDuration
      delay:0
      options:UIViewAnimationOptionCurveEaseInOut
      animations:^{
        [[self.popupContainerView superview] layoutIfNeeded];
      }
      completion:^(BOOL) {
        [retainedPopupView removeFromSuperview];
      }];
}

+ (NSString*)layoutGuideIdentifier {
  return @"omniboxPopupLayoutGuide";
}

#pragma mark - Background creation

+ (UIView*)newBackgroundViewIpad {
  UIView* view = [[UIView alloc] init];
  [view setClipsToBounds:YES];

  [view setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
  UIImageView* shadowView = [[UIImageView alloc]
      initWithImage:NativeImage(IDR_IOS_TOOLBAR_SHADOW_FULL_BLEED)];
  [shadowView setUserInteractionEnabled:NO];
  [shadowView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [view addSubview:shadowView];

  // Add constraints to position |shadowView| at the bottom of |view|
  // with the same width as |view|.
  NSDictionary* views = NSDictionaryOfVariableBindings(shadowView);
  [view addConstraints:[NSLayoutConstraint
                           constraintsWithVisualFormat:@"H:|[shadowView]|"
                                               options:0
                                               metrics:nil
                                                 views:views]];
  [view addConstraint:[NSLayoutConstraint
                          constraintWithItem:shadowView
                                   attribute:NSLayoutAttributeBottom
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:view
                                   attribute:NSLayoutAttributeBottom
                                  multiplier:1
                                    constant:0]];

  return view;
}

+ (UIView*)newBackgroundViewIPhone {
  UIView* view = [[UIView alloc] init];

  // Add a white background to prevent seeing the logo scroll through the
  // omnibox.
  UIView* whiteBackground = [[UIView alloc] initWithFrame:CGRectZero];
  [view addSubview:whiteBackground];
  [whiteBackground setBackgroundColor:[UIColor whiteColor]];

  // Set constraints to |whiteBackground|.
  [whiteBackground setTranslatesAutoresizingMaskIntoConstraints:NO];
  NSDictionary* metrics = @{ @"height" : @(kWhiteBackgroundHeight) };
  NSDictionary* views = NSDictionaryOfVariableBindings(whiteBackground);
  [view addConstraints:[NSLayoutConstraint
                           constraintsWithVisualFormat:@"H:|[whiteBackground]|"
                                               options:0
                                               metrics:nil
                                                 views:views]];
  [view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:
                                               @"V:[whiteBackground(==height)]"
                                                               options:0
                                                               metrics:metrics
                                                                 views:views]];
  [view addConstraint:[NSLayoutConstraint
                          constraintWithItem:whiteBackground
                                   attribute:NSLayoutAttributeBottom
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:view
                                   attribute:NSLayoutAttributeTop
                                  multiplier:1
                                    constant:0]];
  return view;
}

@end
