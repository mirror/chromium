// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/find_in_page/find_in_page_view_controller.h"

#include "base/format_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/find_bar/find_bar_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

namespace {
// For the first |kSearchDelayChars| characters, delay by |kSearchLongDelay|
// For the remaining characters, delay by |kSearchShortDelay|.
const NSUInteger kSearchDelayChars = 3;
const NSTimeInterval kSearchLongDelay = 1.0;
const NSTimeInterval kSearchShortDelay = 0.100;
}  // namespace

@interface FindInPageViewController ()
// The FindBarView owned by this view controller.
@property(nonatomic, readwrite, strong) FindBarView* findBarView;

// Typing delay timer.
@property(nonatomic, strong) NSTimer* delayTimer;
@end

@implementation FindInPageViewController

@dynamic view;
@synthesize delayTimer = _delayTimer;
@synthesize dispatcher = _dispatcher;
@synthesize findBarView = _findBarView;

- (void)loadView {
  self.view = [[UIView alloc] init];
}

- (void)viewDidLoad {
  // Set up the background (an image on tablet, a solid color on phone).
  if (IsIPadIdiom()) {
    UIImageView* backgroundView = [[UIImageView alloc] init];
    backgroundView.frame = self.view.bounds;
    backgroundView.autoresizingMask =
        UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

    UIEdgeInsets backgroundInsets = UIEdgeInsetsMake(6, 9, 10, 8);
    UIImage* backgroundImage = [UIImage imageNamed:@"find_bg"];
    backgroundView.image =
        [backgroundImage resizableImageWithCapInsets:backgroundInsets];

    [self.view addSubview:backgroundView];
    self.view.opaque = NO;

    // The background image view has an intrinsic size and is pinned to fill its
    // parent.
    backgroundView.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
      [backgroundView.leadingAnchor
          constraintEqualToAnchor:self.view.leadingAnchor],
      [backgroundView.trailingAnchor
          constraintEqualToAnchor:self.view.trailingAnchor],
      [backgroundView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
      [backgroundView.bottomAnchor
          constraintEqualToAnchor:self.view.bottomAnchor],
    ]];
  } else {
    self.view.backgroundColor = [UIColor whiteColor];
    self.view.opaque = YES;
  }

  // Add the FindBarView next, so that it is z-ordered above the background.
  FindBarView* findBarView = [[FindBarView alloc] initWithDarkAppearance:NO];
  findBarView.frame = self.view.bounds;
  findBarView.autoresizingMask =
      UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  self.findBarView = findBarView;
  [self.view addSubview:self.findBarView];

  // Set up constraints.
  findBarView.translatesAutoresizingMaskIntoConstraints = NO;
  if (IsIPadIdiom()) {
    // On tablet, an intrinsic height is derived from the background image, but
    // width is determined by the parent. The FindBarView is centered vertically
    // in its parent.
    [NSLayoutConstraint activateConstraints:@[
      // The FindBarView is centered in its parent, but shifted up slightly
      // to account for the fact that the background image is slightly
      // off-center.
      [self.view.centerYAnchor constraintEqualToAnchor:findBarView.centerYAnchor
                                              constant:2],
      [self.view.leadingAnchor
          constraintEqualToAnchor:findBarView.leadingAnchor],
      [self.view.trailingAnchor
          constraintEqualToAnchor:findBarView.trailingAnchor],
      [self.findBarView.heightAnchor constraintEqualToConstant:56.0f]
    ]];

  } else {
    // On phone, both width and height are determined by the parent.  The
    // FindBarView is sized to fill its parent.
    [NSLayoutConstraint activateConstraints:@[
      [findBarView.leadingAnchor
          constraintEqualToAnchor:self.view.leadingAnchor],
      [findBarView.trailingAnchor
          constraintEqualToAnchor:self.view.trailingAnchor],
      [findBarView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
      [findBarView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];
  }

  // Set up button actions.
  [findBarView.inputField addTarget:self
                             action:@selector(findBarTextDidChange)
                   forControlEvents:UIControlEventEditingChanged];
  [findBarView.nextButton addTarget:self
                             action:@selector(findNextInPage)
                   forControlEvents:UIControlEventTouchUpInside];
  [findBarView.previousButton addTarget:self
                                 action:@selector(findPreviousInPage)
                       forControlEvents:UIControlEventTouchUpInside];
  [findBarView.closeButton addTarget:self
                              action:@selector(hideFindInPage)
                    forControlEvents:UIControlEventTouchUpInside];
}

- (void)setCurrentMatch:(NSUInteger)current ofTotalMatches:(NSUInteger)total {
  NSString* indexStr = [NSString stringWithFormat:@"%" PRIdNS, current];
  NSString* matchesStr = [NSString stringWithFormat:@"%" PRIdNS, total];
  NSString* text = l10n_util::GetNSStringF(
      IDS_FIND_IN_PAGE_COUNT, base::SysNSStringToUTF16(indexStr),
      base::SysNSStringToUTF16(matchesStr));
  [self.findBarView updateResultsLabelWithText:text];
}

- (void)textChanged {
  [self.dispatcher findStringInPage:self.findBarView.inputField.text];
}

#pragma mark - Actions

- (void)showFindInPage {
  [self.dispatcher showFindInPage];
}

- (void)findNextInPage {
  [self.dispatcher findNextInPage];
}

- (void)findPreviousInPage {
  [self.dispatcher findPreviousInPage];
}

- (void)hideFindInPage {
  [self.dispatcher hideFindInPage];
}

- (void)findBarTextDidChange {
  [self.delayTimer invalidate];
  NSUInteger length = [self.findBarView.inputField.text length];
  if (length == 0)
    return [self textChanged];

  // Delay delivery of text change event.  Use a longer delay when the input
  // length is short.
  NSTimeInterval delay =
      (length > kSearchDelayChars) ? kSearchShortDelay : kSearchLongDelay;
  self.delayTimer =
      [NSTimer scheduledTimerWithTimeInterval:delay
                                       target:self
                                     selector:@selector(textChanged)
                                     userInfo:nil
                                      repeats:NO];
}

@end
