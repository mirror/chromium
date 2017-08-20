// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/experiment_support.h"

#import <Foundation/Foundation.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace experiment_support {

bool IsTapStripEnabled() {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* tabStripPreference = [defaults stringForKey:@"EnableTabStrip"];
  return [tabStripPreference isEqualToString:@"Enabled"];
}

bool IsBottomToolbarEnabled() {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* bottomToolbarPreference =
      [defaults stringForKey:@"EnableBottomToolbar"];
  return [bottomToolbarPreference isEqualToString:@"Enabled"];
}

}  // namespace experiment_support
