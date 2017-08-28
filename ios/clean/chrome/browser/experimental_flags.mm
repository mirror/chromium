// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/experimental_flags.h"

#import <Foundation/Foundation.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kEnableTabStrip = @"EnableTabStrip";
NSString* const kEnableBottomToolbar = @"EnableBottomToolbar";
}  // namespace

namespace experimental_flags {

bool IsTapStripEnabled() {
  return [[NSUserDefaults standardUserDefaults] boolForKey:kEnableTabStrip];
}

bool IsBottomToolbarEnabled() {
  return
      [[NSUserDefaults standardUserDefaults] boolForKey:kEnableBottomToolbar];
}

}  // namespace experimental_flags
