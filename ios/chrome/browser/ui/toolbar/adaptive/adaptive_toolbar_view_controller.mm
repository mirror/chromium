// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view_controller.h"

#import "base/logging.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/primary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/adaptive/secondary_toolbar_view.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_button_factory.h"
#import "ios/chrome/browser/ui/toolbar/clean/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_constants.h"
#import "ios/third_party/material_components_ios/src/components/ProgressView/src/MaterialProgressView.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

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
// Whether a page is loading.
@property(nonatomic, assign, getter=isLoading) BOOL loading;

@end

@implementation AdaptiveToolbarViewController
@dynamic view;
@synthesize buttonFactory = _buttonFactory;
@synthesize dispatcher = _dispatcher;
@synthesize loading = _loading;
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

#pragma mark - ToolbarConsumer

- (void)setCanGoForward:(BOOL)canGoForward {
  self.view.forwardLeadingButton.enabled = canGoForward;
  self.view.forwardTrailingButton.enabled = canGoForward;
}

- (void)setCanGoBack:(BOOL)canGoBack {
  self.view.backButton.enabled = canGoBack;
}

- (void)setLoadingState:(BOOL)loading {
  self.loading = loading;
  self.view.reloadButton.hiddenInCurrentState = loading;
  self.view.stopButton.hiddenInCurrentState = !loading;
  if (!loading) {
    [self stopProgressBar];
  } else if (self.view.progressBar.hidden) {
    [self.view.progressBar setProgress:0];
    [self.view.progressBar setHidden:NO animated:YES completion:nil];
    // Layout if needed the progress bar to avoir having the progress bar going
    // backward when opening a page from the NTP.
    [self.view.progressBar layoutIfNeeded];
  }
}

- (void)setLoadingProgressFraction:(double)progress {
  [self.view.progressBar setProgress:progress animated:YES completion:nil];
}

- (void)setTabCount:(int)tabCount {
  // Return if tabGridButton wasn't initialized.
  if (!self.view.tabGridButton)
    return;

  // Update the text shown in the |self.view.tabGridButton|. Note that
  // the button's title may be empty or contain an easter egg, but the
  // accessibility value will always be equal to |tabCount|.
  NSString* tabStripButtonValue = [NSString stringWithFormat:@"%d", tabCount];
  NSString* tabStripButtonTitle;
  id<MDCTypographyFontLoading> fontLoader = [MDCTypography fontLoader];
  if (tabCount <= 0) {
    tabStripButtonTitle = @"";
  } else if (tabCount > kShowTabStripButtonMaxTabCount) {
    // As an easter egg, show a smiley face instead of the count if the user has
    // more than 99 tabs open.
    tabStripButtonTitle = @":)";
    [[self.view.tabGridButton titleLabel]
        setFont:[fontLoader boldFontOfSize:kFontSizeFewerThanTenTabs]];
  } else {
    tabStripButtonTitle = tabStripButtonValue;
    if (tabCount < 10) {
      [[self.view.tabGridButton titleLabel]
          setFont:[fontLoader boldFontOfSize:kFontSizeFewerThanTenTabs]];
    } else {
      [[self.view.tabGridButton titleLabel]
          setFont:[fontLoader boldFontOfSize:kFontSizeTenTabsOrMore]];
    }
  }

  [self.view.tabGridButton setTitle:tabStripButtonTitle
                           forState:UIControlStateNormal];
  [self.view.tabGridButton setAccessibilityValue:tabStripButtonValue];
}

- (void)setPageBookmarked:(BOOL)bookmarked {
  self.view.bookmarkButton.selected = bookmarked;
}

- (void)setVoiceSearchEnabled:(BOOL)enabled {
  // No-op, should be handled by the location bar.
}

- (void)setShareMenuEnabled:(BOOL)enabled {
  self.view.shareButton.enabled = enabled;
}

#pragma mark - Private

// Updates all buttons visibility to match any recent WebState or SizeClass
// change.
- (void)updateAllButtonsVisibility {
  for (ToolbarButton* button in self.view.allButtons) {
    [button updateHiddenInCurrentSizeClass];
  }
}

// Sets the progress of the progressBar to 1 then hides it.
- (void)stopProgressBar {
  __weak MDCProgressView* weakProgressBar = self.view.progressBar;
  __weak AdaptiveToolbarViewController* weakSelf = self;
  [self.view.progressBar
      setProgress:1
         animated:YES
       completion:^(BOOL finished) {
         if (!weakSelf.loading) {
           [weakProgressBar setHidden:YES animated:YES completion:nil];
         }
       }];
}

@end
