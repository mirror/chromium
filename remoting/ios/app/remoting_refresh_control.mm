// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/remoting_refresh_control.h"

#include "base/logging.h"
#include "base/numerics/math_constants.h"

#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MDCActivityIndicator.h"
#import "remoting/ios/app/remoting_theme.h"

static const CGFloat kViewHeightRatioToTriggerRefresh = 0.08f;
static const CGFloat kRefreshIconHeight = RemotingTheme.refreshIcon.size.height;
static const CGFloat kRefreshingViewHeight = 26.f;

static void* kCRDRefreshContentOffsetContext = &kCRDRefreshContentOffsetContext;
static void* kCRDRefreshContentInsetContext = &kCRDRefreshContentInsetContext;

@implementation RemotingRefreshControl {
  UIImageView* _refreshIconView;
  __weak UIScrollView* _trackingScrollView;
  BOOL _isRefreshing;
  MDCActivityIndicator* _activityIndicator;
}

@synthesize refreshCallback = _refreshCallback;

#pragma mark - UIView

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    _refreshIconView =
        [[UIImageView alloc] initWithImage:RemotingTheme.refreshIcon];
    _refreshIconView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_refreshIconView];
    [NSLayoutConstraint activateConstraints:@[
      [_refreshIconView.centerXAnchor
          constraintEqualToAnchor:self.centerXAnchor],
      [_refreshIconView.topAnchor constraintEqualToAnchor:self.topAnchor],
      [_refreshIconView.widthAnchor constraintEqualToConstant:kRefreshIconHeight],
      [_refreshIconView.heightAnchor constraintEqualToConstant:kRefreshIconHeight],
    ]];

    _activityIndicator = [[MDCActivityIndicator alloc] init];
    _activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    _activityIndicator.cycleColors = @[ UIColor.whiteColor ];
    _activityIndicator.radius = 10;
    _activityIndicator.strokeWidth = 2.f;
    _activityIndicator.hidden = YES;
    [self addSubview:_activityIndicator];
    [NSLayoutConstraint activateConstraints:@[
      [_activityIndicator.centerXAnchor
          constraintEqualToAnchor:self.centerXAnchor],
      [_activityIndicator.topAnchor
          constraintEqualToAnchor:self.topAnchor],
    ]];

    _isRefreshing = NO;
  }
  return self;
}

- (void)dealloc {
  [self setTrackingScrollView:nil];
}

- (void)willMoveToSuperview:(UIView*)newSuperview {
  DCHECK([newSuperview isKindOfClass:[UIScrollView class]]);
  [self setTrackingScrollView:(UIScrollView*)newSuperview];
}

- (void)layoutSubviews {
  [super layoutSubviews];

  if (!_trackingScrollView) {
    return;
  }

  CGFloat controlViewHeight = [self controlViewHeight];
  self.frame =
      CGRectMake(0, -controlViewHeight, _trackingScrollView.contentSize.width,
                 controlViewHeight);
  CGFloat progress = [self pullProgress];
  _refreshIconView.alpha = progress;
  _refreshIconView.transform =
      CGAffineTransformMakeRotation(progress * 2 * base::kPiFloat);
}

#pragma mark - Private

- (void)setTrackingScrollView:(UIScrollView*)scrollView {
  if (_trackingScrollView) {
    [_trackingScrollView.panGestureRecognizer removeTarget:self action:NULL];
  }
  _trackingScrollView = scrollView;
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
      [self pullProgress] == 1 && _refreshCallback) {
    [self setRefreshing:YES];
//    [self setNeedsLayout];
    //    _refreshCallback();
  }
}

- (void)setRefreshing:(BOOL)isRefreshing {
  if (_isRefreshing == isRefreshing || !_trackingScrollView) {
    return;
  }

  _isRefreshing = isRefreshing;
//  CGFloat heightAdjustment =
//      _isRefreshing ? kRefreshingViewHeight : -kRefreshingViewHeight;
//    _trackingScrollView.contentInset =
//      UIEdgeInsetsMake(_trackingScrollView.contentInset.top + heightAdjustment,
//                       _trackingScrollView.contentInset.left,
//                       _trackingScrollView.contentInset.bottom,
//                       _trackingScrollView.contentInset.right);
//    _trackingScrollView.contentOffset =
//        CGPointMake(_trackingScrollView.contentOffset.x,
//                    -_trackingScrollView.contentInset.top);

  if (_isRefreshing) {
    _activityIndicator.hidden = NO;
    _refreshIconView.hidden = YES;
    [_activityIndicator startAnimating];
  } else {
    [_activityIndicator stopAnimating];
    _activityIndicator.hidden = YES;
    _refreshIconView.hidden = NO;
  }
}

// Calculates control view's height from the scroll view's offset and inset.
- (CGFloat)controlViewHeight {
  CGFloat topInsetBeforeAdjustment = _trackingScrollView.contentInset.top;
  if (_isRefreshing) {
    topInsetBeforeAdjustment -= kRefreshingViewHeight;
  }
  return -_trackingScrollView.contentOffset.y - topInsetBeforeAdjustment;
}

// Returns a number between [0, 1] indicating the progress of the user pulling
// down the scroll view. Used to determine alpha and rotation.
- (CGFloat)pullProgress {
  CGFloat progress = ([self controlViewHeight] - kRefreshIconHeight) /
                     (kViewHeightRatioToTriggerRefresh *
                      _trackingScrollView.frame.size.height);
  if (progress < 0)
    return 0;
  if (progress > 1)
    return 1;
  // Ease-out effect.
  return sqrt(progress);
}

@end
