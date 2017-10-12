// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_CHROMEOS_TOUCHSCREEN_UTIL_H_
#define UI_DISPLAY_MANAGER_CHROMEOS_TOUCHSCREEN_UTIL_H_

#include <vector>

#include "ui/display/manager/display_manager_export.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/events/devices/touchscreen_device.h"

namespace display {
class DisplayManager;

// Given a list of displays and a list of touchscreens, associate them. The
// information in |displays| will be updated to reflect the mapping.
DISPLAY_MANAGER_EXPORT void AssociateTouchscreens(
    std::vector<ManagedDisplayInfo>* displays,
    const std::vector<ui::TouchscreenDevice>& touchscreens,
    const DisplayManager* display_manager);

// Returns true if the touchscreen identified by |touch_device_identifier|, an
// internal touch screen device.
DISPLAY_MANAGER_EXPORT bool IsInternalTouchscreenDevice(
    uint32_t touch_device_identifier);

// Returns true if the system has any external touch devices attached to it.
DISPLAY_MANAGER_EXPORT bool HasExternalTouchscreenDevice();
}  // namespace display

#endif  // UI_DISPLAY_MANAGER_CHROMEOS_TOUCHSCREEN_UTIL_H_
