// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NewTabPageView

@synthesize tabBar = tabBar_;
@synthesize safeAreaInsetForToolbar = _safeAreaInsetForToolbar;

- (instancetype)initWithFrame:(CGRect)frame
                    andTabBar:(NewTabPageBar*)tabBar {
  self = [super initWithFrame:frame];
  if (self) {
    [self addSubview:tabBar];
    tabBar_ = tabBar;
  }
  return self;
}

- (void)safeAreaInsetsDidChange {
  self.safeAreaInsetForToolbar = self.safeAreaInsets;
  [super safeAreaInsetsDidChange];
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

#pragma mark - Properties

- (void)setSafeAreaInsetForToolbar:(UIEdgeInsets)safeAreaInsetForToolbar {
  _safeAreaInsetForToolbar = safeAreaInsetForToolbar;
  self.tabBar.safeAreaInsetFromNTPView = safeAreaInsetForToolbar;
}

- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];

  // This should never be needed in autolayout.
  if (self.translatesAutoresizingMaskIntoConstraints) {
    // Trigger a layout.  The |-layoutIfNeeded| call is required because
    // sometimes  |-layoutSubviews| is not successfully triggered when
    // |-setNeedsLayout| is called after frame changes due to autoresizing
    // masks.
    [self setNeedsLayout];
    [self layoutIfNeeded];
  }
}

@end
