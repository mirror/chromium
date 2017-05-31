// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/touch_bar_util.h"
#include "base/mac/foundation_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"

namespace ui {

NSButton* CreateBlueTouchBarButton(NSString* title, id target, SEL action) {
  NSButton* button =
      [NSButton buttonWithTitle:title target:target action:action];
  [button setBezelColor:[NSColor colorWithSRGBRed:0.168
                                            green:0.51
                                             blue:0.843
                                            alpha:1.0]];
  return button;
}

NSString* CreateTouchBarId(NSString* touch_bar_id) {
  NSString* chrome_bundle_id =
      base::SysUTF8ToNSString(base::mac::BaseBundleID());
  return [NSString stringWithFormat:@"%@.%@", chrome_bundle_id, touch_bar_id];
}

NSString* CreateTouchBarItemId(NSString* touch_bar_id, NSString* item_id) {
  return [NSString
      stringWithFormat:@"%@-%@", CreateTouchBarId(touch_bar_id), item_id];
}

}  // namespace ui
