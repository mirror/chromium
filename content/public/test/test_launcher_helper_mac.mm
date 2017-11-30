// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/test_launcher_helper_mac.h"

#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>
#include <tuple>

#include "base/mac/scoped_cftyperef.h"

namespace {

NSUInteger GetCurrentModifierFlags() {
  return
      [NSEvent modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
}

std::tuple<NSUInteger, int, const char*> kModifierFlagToKeyCodeMap[] = {
    {NSEventModifierFlagCommand, kVK_Command, "Cmd"},
    {NSEventModifierFlagShift, kVK_Shift, "Shift"},
    {NSEventModifierFlagOption, kVK_Option, "Option"},
    // Expand as needed.
};

}  // namespace

void ClearActiveModifiers() {
  const NSUInteger active_modifiers = GetCurrentModifierFlags();
  for (auto flag_key_code : kModifierFlagToKeyCodeMap) {
    if (active_modifiers & std::get<0>(flag_key_code)) {
      LOG(ERROR) << "Modifier is hanging: " << std::get<2>(flag_key_code);
      CGEventPost(kCGSessionEventTap,
                  base::ScopedCFTypeRef<CGEventRef>(CGEventCreateKeyboardEvent(
                      nullptr, std::get<1>(flag_key_code), false)));
    }
  }
}
