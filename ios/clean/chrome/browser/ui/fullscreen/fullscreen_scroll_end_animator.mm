// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"

#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FullscreenScrollEndTimingCurveProvider
    : NSObject<UITimingCurveProvider>
@end

@implementation FullscreenScrollEndTimingCurveProvider

- (UITimingCurveType)timingCurveType {
  return UITimingCurveTypeCubic;
}

- (UICubicTimingParameters*)cubicTimingParameters {
  // Control points for MaterialDesign CurveEaseOut curve.
  return [[UICubicTimingParameters alloc]
      initWithControlPoint1:CGPointZero
              controlPoint2:CGPointMake(0.2f, 0.1f)];
}

- (UISpringTimingParameters*)springTimingParameters {
  return nil;
}

#pragma mark NSCoding

- (void)encodeWithCoder:(NSCoder*)aCoder {
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  return [[FullscreenScrollEndTimingCurveProvider alloc] init];
}

#pragma mark NSCopying

- (instancetype)copyWithZone:(NSZone*)zone {
  return [[FullscreenScrollEndTimingCurveProvider alloc] init];
}

@end

@implementation FullscreenScrollEndAnimator
@synthesize finalProgress = _finalProgress;

- (instancetype)initForVisibleToolbar:(BOOL)visible {
  FullscreenScrollEndTimingCurveProvider* curveProvider =
      [[FullscreenScrollEndTimingCurveProvider alloc] init];
  self = [super initWithDuration:ios::material::kDuration1
                timingParameters:curveProvider];
  if (self) {
    _finalProgress = visible ? 1.0 : 0.0;
  }
  return self;
}

#pragma mark - UIViewPropertyAnimator

- (BOOL)isInterruptible {
  return YES;
}

@end
