// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/incognito_view_controller.h"

#include <string>

#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/ntp/incognito_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/url_loader.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kDistanceToFadeToolbar = 50.0;
}  // namespace

@interface IncognitoViewController ()<UIScrollViewDelegate>
@property(nonatomic, weak) id<BrowserCommands> dispatcher;
@end

@implementation IncognitoViewController {
  // The scrollview containing the actual views.
  IncognitoView* _incognitoView;
}

@synthesize dispatcher = _dispatcher;

// Property declared in NewTabPagePanelProtocol.
@synthesize delegate = _delegate;

- (id)initWithLoader:(id<UrlLoader>)loader
          dispatcher:(id<BrowserCommands>)dispatcher {
  self = [super init];
  if (self) {
    _incognitoView = [[IncognitoView alloc]
        initWithFrame:[UIApplication sharedApplication].keyWindow.bounds
            urlLoader:loader];
    [_incognitoView setAutoresizingMask:UIViewAutoresizingFlexibleHeight |
                                        UIViewAutoresizingFlexibleWidth];

    if (IsIPadIdiom()) {
      [_incognitoView setBackgroundColor:[UIColor clearColor]];
    } else {
      [_incognitoView
          setBackgroundColor:[UIColor colorWithWhite:34 / 255.0 alpha:1.0]];
      [_incognitoView setDelegate:self];
      _dispatcher = dispatcher;
      [_dispatcher setToolbarBackgroundAlpha:0];
    }
  }
  return self;
}

- (void)viewDidLoad {
  [self.view addSubview:_incognitoView];
}

- (void)dealloc {
  [self.dispatcher setToolbarBackgroundAlpha:1];
  [_incognitoView setDelegate:nil];
}

#pragma mark - NewTabPagePanelProtocol

- (void)reload {
}

- (void)wasShown {
  CGFloat alpha = [self toolbarAlphaForScrollView:_incognitoView];
  [self.dispatcher setToolbarBackgroundAlpha:alpha];
}

- (void)wasHidden {
  [self.dispatcher setToolbarBackgroundAlpha:1];
}

- (void)dismissModals {
}

- (void)dismissKeyboard {
}

- (void)setScrollsToTop:(BOOL)enable {
}

- (CGFloat)alphaForBottomShadow {
  return 0;
}

#pragma mark - UIScrollViewDelegate methods

- (void)scrollViewDidScroll:(UIScrollView*)scrollView {
  CGFloat alpha = [self toolbarAlphaForScrollView:_incognitoView];
  [self.dispatcher setToolbarBackgroundAlpha:alpha];
}

#pragma mark - Private

// Calculate the background alpha for the toolbar based on how much |scrollView|
// has scrolled up.
- (CGFloat)toolbarAlphaForScrollView:(UIScrollView*)scrollView {
  CGFloat alpha = scrollView.contentOffset.y / kDistanceToFadeToolbar;
  return MAX(MIN(alpha, 1), 0);
}

@end
