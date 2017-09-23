// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/remoting_refresh_control.h"

#include "base/logging.h"
#include "base/numerics/math_constants.h"

#import "remoting/ios/app/remoting_theme.h"

static const CGFloat kViewHeightRatioToTriggerRefresh = 0.08f;
static const CGFloat kIndicatorRadius = 9.33f;
static const CGFloat kIndicatorStrokeWidth = 2.33f;
static const CGFloat kRefreshIconHeight = 28.f;
static const CGFloat kContentAreaHeight = 30.f;

static void* kCRDRefreshContentOffsetContext = &kCRDRefreshContentOffsetContext;
static void* kCRDRefreshContentInsetContext = &kCRDRefreshContentInsetContext;

@implementation RemotingRefreshControl {
  UIImageView* _refreshIconView;
  __weak UIScrollView* _trackingScrollView;
  BOOL _isRefreshing;
  MDCActivityIndicator* _activityIndicator;
}

@synthesize isRefreshing = _isRefreshing;
@synthesize refreshCallback = _refreshCallback;
@synthesize trackingScrollView = _trackingScrollView;

#pragma mark - UIView

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    UILayoutGuide* contentAreaGuide = [[UILayoutGuide alloc] init];
    [self addLayoutGuide:contentAreaGuide];
    [NSLayoutConstraint activateConstraints:@[
      [contentAreaGuide.leadingAnchor
          constraintEqualToAnchor:self.leadingAnchor],
      [contentAreaGuide.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor],
      [contentAreaGuide.topAnchor constraintEqualToAnchor:self.topAnchor],
      [contentAreaGuide.heightAnchor
          constraintEqualToConstant:kContentAreaHeight],
    ]];

    _refreshIconView =
        [[UIImageView alloc] initWithImage:RemotingTheme.refreshIcon];
    _refreshIconView.alpha = 0;
    _refreshIconView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_refreshIconView];
    [NSLayoutConstraint activateConstraints:@[
      [_refreshIconView.centerXAnchor
          constraintEqualToAnchor:contentAreaGuide.centerXAnchor],
      [_refreshIconView.centerYAnchor
          constraintEqualToAnchor:contentAreaGuide.centerYAnchor],
      [_refreshIconView.widthAnchor
          constraintEqualToConstant:kRefreshIconHeight],
      [_refreshIconView.heightAnchor
          constraintEqualToConstant:kRefreshIconHeight],
    ]];

    _activityIndicator = [RemotingRefreshControl createRefreshIndicator];
    _activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_activityIndicator];
    [NSLayoutConstraint activateConstraints:@[
      [_activityIndicator.centerXAnchor
          constraintEqualToAnchor:contentAreaGuide.centerXAnchor],
      [_activityIndicator.centerYAnchor
          constraintEqualToAnchor:contentAreaGuide.centerYAnchor],
    ]];

    _isRefreshing = NO;
  }
  return self;
}

- (void)dealloc {
  self.trackingScrollView = nil;
}

- (void)layoutSubviews {
  [super layoutSubviews];

  if (!_trackingScrollView) {
    return;
  }

  CGFloat controlViewHeight = [self controlViewHeight];
  if (controlViewHeight <= 0) {
    return;
  }

  self.frame =
      CGRectMake(0, -controlViewHeight, _trackingScrollView.contentSize.width,
                 controlViewHeight);

  if (_isRefreshing) {
    _refreshIconView.alpha = 0;
    return;
  }

  CGFloat progress = [self pullProgress];
  _refreshIconView.alpha = progress;
  _refreshIconView.transform =
      CGAffineTransformMakeRotation(progress * 2 * base::kPiFloat);
}

#pragma mark - Public

- (void)endRefreshing {
  [self setRefreshing:NO];
}

+ (MDCActivityIndicator*)createRefreshIndicator {
  MDCActivityIndicator* indicator = [[MDCActivityIndicator alloc] init];
  indicator.cycleColors = RemotingTheme.refreshIndicatorColors;
  indicator.radius = kIndicatorRadius;
  indicator.strokeWidth = kIndicatorStrokeWidth;
  return indicator;
}

#pragma mark - Private

- (void)setTrackingScrollView:(UIScrollView*)scrollView {
  [self setRefreshing:NO];
  if (_trackingScrollView) {
    [_trackingScrollView.panGestureRecognizer removeTarget:self action:NULL];
    [self removeFromSuperview];
  }
  _trackingScrollView = scrollView;
  [_trackingScrollView addSubview:self];

  if (_trackingScrollView) {
    [_trackingScrollView.panGestureRecognizer
        addTarget:self
           action:@selector(onPanGestureTriggered:)];

    // layoutSubviews is not triggered when contentOffset is negative. These
    // KVO are used to handle that.
    [_trackingScrollView addObserver:self
                          forKeyPath:@"contentOffset"
                             options:NSKeyValueObservingOptionNew |
                                     NSKeyValueObservingOptionInitial
                             context:kCRDRefreshContentOffsetContext];

    [_trackingScrollView addObserver:self
                          forKeyPath:@"contentInset"
                             options:NSKeyValueObservingOptionNew |
                                     NSKeyValueObservingOptionInitial
                             context:kCRDRefreshContentInsetContext];
  }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
  if (context == kCRDRefreshContentInsetContext ||
      context == kCRDRefreshContentOffsetContext) {
    if (_trackingScrollView.contentOffset.y < 0) {
      [self setNeedsLayout];
    }
    return;
  }

  // Any unrecognized context must belong to super
  [super observeValueForKeyPath:keyPath
                       ofObject:object
                         change:change
                        context:context];
}

- (void)onPanGestureTriggered:(UIPanGestureRecognizer*)sender {
  if (sender.state == UIGestureRecognizerStateEnded &&
      [self pullProgress] == 1) {
    [self setRefreshing:YES];

    if (_refreshCallback) {
      _refreshCallback();
    }
  }
}

- (void)setRefreshing:(BOOL)isRefreshing {
  if (_isRefreshing == isRefreshing || !_trackingScrollView) {
    return;
  }
  [self layoutIfNeeded];

  _isRefreshing = isRefreshing;
  UIEdgeInsets targetInset = _trackingScrollView.contentInset;
  targetInset.top += _isRefreshing ? kContentAreaHeight : -kContentAreaHeight;

  if (_isRefreshing) {
    _activityIndicator.hidden = NO;
    [_activityIndicator startAnimating];
  } else {
    _activityIndicator.hidden = YES;
    [_activityIndicator stopAnimating];
  }

  [UIView animateWithDuration:0.2f
                        delay:0
                      options:UIViewAnimationOptionAllowUserInteraction |
                              UIViewAnimationOptionBeginFromCurrentState
                   animations:^() {
                     _trackingScrollView.contentInset = targetInset;
                   }
                   completion:nil];
}

// Calculates control view's height from the scroll view's offset and inset.
- (CGFloat)controlViewHeight {
  CGFloat topInsetBeforeAdjustment = _trackingScrollView.contentInset.top;
  if (_isRefreshing) {
    topInsetBeforeAdjustment -= kContentAreaHeight;
  }
  return -_trackingScrollView.contentOffset.y - topInsetBeforeAdjustment;
}

// Returns a number between [0, 1] indicating the progress of the user pulling
// down the scroll view. Used to determine alpha and rotation.
- (CGFloat)pullProgress {
  CGFloat progress = ([self controlViewHeight] - kRefreshIconHeight) /
                     (kViewHeightRatioToTriggerRefresh *
                      _trackingScrollView.frame.size.height);
  // Ease-out effect.
  return sqrt(MAX(0, MIN(1, progress)));
}

@end
