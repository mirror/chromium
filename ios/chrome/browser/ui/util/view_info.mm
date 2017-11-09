// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/view_info.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

ViewInfo::ViewInfo(UIView* view) : view_(view) {}
ViewInfo::~ViewInfo() = default;

CGRect ViewInfo::GetFrameInView(UIView* view) const {
  return [view convertRect:view_.bounds fromView:view_];
}

CGPoint ViewInfo::GetCenterInView(UIView* view) const {
  CGRect frame = GetFrameInView(view);
  return CGPointMake(CGRectGetMidX(frame), CGRectGetMidY(frame));
}
