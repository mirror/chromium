// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/find_in_page/find_in_page_metrics_recorder.h"

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#import "ios/clean/chrome/browser/ui/commands/find_in_page_visibility_commands.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation FindInPageMetricsRecorder

- (void)recordMetricForInvocation:(NSInvocation*)anInvocation {
  // Ensure that |anInvocation|'s selector is |showFindInPage|, as that is the
  // only command this recorder handles.
  DCHECK_EQ(anInvocation.selector, @selector(showFindInPage));

  base::RecordAction(base::UserMetricsAction("MobileMenuFindInPage"));
}

@end
