// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/refresh_control_provider_chromium.h"

#import "remoting/ios/app/remoting_theme.h"

@interface RemotingRefreshControlChromium
    : UIRefreshControl<RemotingRefreshControl>
- (instancetype)initWithScrollView:(UIScrollView*)scrollView
                       actionBlock:(RemotingRefreshAction)actionBlock;
@end

@implementation RemotingRefreshControlChromium {
  RemotingRefreshAction _refreshAction;
}

- (instancetype)initWithScrollView:(UIScrollView*)scrollView
                       actionBlock:(RemotingRefreshAction)actionBlock {
  if (self = [super initWithFrame:CGRectZero]) {
    self.tintColor = RemotingTheme.refreshIndicatorColor;
    [scrollView addSubview:self];
    _refreshAction = actionBlock;
    [self addTarget:self
                  action:@selector(onRefreshTriggered)
        forControlEvents:UIControlEventValueChanged];
  }
  return self;
}

- (void)onRefreshTriggered {
  _refreshAction();
}

@end

@implementation RefreshControlProviderChromium

- (id<RemotingRefreshControl>)createForScrollView:(UIScrollView*)scrollView
                                      actionBlock:
                                          (RemotingRefreshAction)action {
  return [[RemotingRefreshControlChromium alloc] initWithScrollView:scrollView
                                                        actionBlock:action];
}

@end
