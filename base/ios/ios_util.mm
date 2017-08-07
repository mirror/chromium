// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/ios/ios_util.h"

#import <Foundation/Foundation.h>
#include <stddef.h>

#include "base/macros.h"
#include "base/sys_info.h"


namespace base {
namespace ios {

bool IsRunningOnIOS10OrLater() {
  return true;
}

bool IsRunningOnIOS11OrLater() {
  return false;
}

bool IsRunningOnOrLater(int32_t major, int32_t minor, int32_t bug_fix) {
  return true;
}

bool IsInForcedRTL() {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  return [defaults boolForKey:@"NSForceRightToLeftWritingDirection"];
}

void OverridePathOfEmbeddedICU(const char* path) {
}

}  // namespace ios
}  // namespace base
