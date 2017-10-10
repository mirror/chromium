// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/public/cpp/hid_usage_and_page.h"

namespace device {

bool IsProtected(const device::mojom::HidUsageAndPagePtr& hid_usage_and_page) {
  const uint16_t usage = hid_usage_and_page->usage;
  const uint16_t usage_page = hid_usage_and_page->usage_page;

  if (usage_page == static_cast<uint16_t>(device::mojom::Page::kPageKeyboard))
    return true;

  if (usage_page !=
      static_cast<uint16_t>(device::mojom::Page::kPageGenericDesktop))
    return false;

  if (usage ==
          static_cast<uint16_t>(
              device::mojom::GenericDesktopUsage::kGenericDesktopPointer) ||
      usage == static_cast<uint16_t>(
                   device::mojom::GenericDesktopUsage::kGenericDesktopMouse) ||
      usage ==
          static_cast<uint16_t>(
              device::mojom::GenericDesktopUsage::kGenericDesktopKeyboard) ||
      usage == static_cast<uint16_t>(
                   device::mojom::GenericDesktopUsage::kGenericDesktopKeypad)) {
    return true;
  }

  if (usage >= static_cast<uint16_t>(device::mojom::GenericDesktopUsage::
                                         kGenericDesktopSystemControl) &&
      usage <= static_cast<uint16_t>(device::mojom::GenericDesktopUsage::
                                         kGenericDesktopSystemWarmRestart)) {
    return true;
  }

  if (usage >=
          static_cast<uint16_t>(
              device::mojom::GenericDesktopUsage::kGenericDesktopSystemDock) &&
      usage <= static_cast<uint16_t>(device::mojom::GenericDesktopUsage::
                                         kGenericDesktopSystemDisplaySwap)) {
    return true;
  }

  return false;
}

}  // namespace device
