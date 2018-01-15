// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/secondary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface AdaptiveToolbarViewController ()

// Button factory.
@property(nonatomic, strong) ToolbarButtonFactory* buttonFactory;
// Redefined to be a PrimaryToolbarView.
@property(nonatomic, strong) UIView<AdaptiveToolbarView>* view;
// Type of this toolbar.
@property(nonatomic, assign) ToolbarType type;

@end

@implementation AdaptiveToolbarViewController
@dynamic view;
@synthesize buttonFactory = _buttonFactory;
@synthesize dispatcher = _dispatcher;
@synthesize type = _type;

#pragma mark - Public

- (instancetype)initWithButtonFactory:(ToolbarButtonFactory*)buttonFactory
                                 type:(ToolbarType)type {
  self = [super initWithNibName:nil bundle:nil];
  if (self) {
    _buttonFactory = buttonFactory;
    _type = type;
  }
  return self;
}

#pragma mark - UIViewController

- (void)loadView {
  switch (self.type) {
    case PRIMARY:
      self.view = [[PrimaryToolbarView alloc] init];
      break;
    case SECONDARY:
      self.view = [[SecondaryToolbarView alloc] init];
      break;
    case LEGACY:
      NOTREACHED();
      break;
  }
  self.view.buttonFactory = self.buttonFactory;
  if (@available(iOS 11, *)) {
    self.view.topSafeAnchor = self.view.safeAreaLayoutGuide.topAnchor;
  } else {
    self.view.topSafeAnchor = self.topLayoutGuide.bottomAnchor;
  }
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  [self updateAllButtonsVisibility];
}

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  [self updateAllButtonsVisibility];
}

#pragma mark - Property accessors

- (void)setLocationBarView:(UIView*)locationBarView {
  self.view.locationBarView = locationBarView;
}

#pragma mark - Private

// Updates all buttons visibility to match any recent WebState or SizeClass
// change.
- (void)updateAllButtonsVisibility {
  for (ToolbarButton* button in self.view.allButtons) {
    [button updateHiddenInCurrentSizeClass];
  }
}

@end
