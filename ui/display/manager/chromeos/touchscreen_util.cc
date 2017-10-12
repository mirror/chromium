// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/chromeos/touchscreen_util.h"

#include <set>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "ui/display/manager/display_manager.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/input_device_manager.h"

namespace display {

namespace {

using DisplayInfoPointerList = std::vector<ManagedDisplayInfo*>;
using DeviceList = std::vector<const ui::TouchscreenDevice*>;

// Helper method to associate |display| and |device|.
void Associate(ManagedDisplayInfo* display,
               const ui::TouchscreenDevice* device) {
  uint32_t touch_device_identifier =
      TouchCalibrationData::GenerateTouchDeviceIdentifier(*device);
  display->AddTouchDevice(touch_device_identifier);
}

// Returns true if |path| is likely a USB device.
bool IsDeviceConnectedViaUsb(const base::FilePath& path) {
  std::vector<base::FilePath::StringType> components;
  path.GetComponents(&components);

  for (const auto& component : components) {
    if (base::StartsWith(component, "usb",
                         base::CompareCase::INSENSITIVE_ASCII))
      return true;
  }

  return false;
}

// Returns the UDL association score between |display| and |device|. A score <=
// 0 means that there is no association.
int GetUdlAssociationScore(const ManagedDisplayInfo* display,
                           const ui::TouchscreenDevice* device) {
  // If the devices are not both connected via USB, then there cannot be a UDL
  // association score.
  if (!IsDeviceConnectedViaUsb(display->sys_path()) ||
      !IsDeviceConnectedViaUsb(device->sys_path))
    return 0;

  // The association score is simply the number of prefix path components that
  // sysfs paths have in common.
  std::vector<base::FilePath::StringType> display_components;
  std::vector<base::FilePath::StringType> device_components;
  display->sys_path().GetComponents(&display_components);
  device->sys_path.GetComponents(&device_components);

  std::size_t largest_idx = 0;
  while (largest_idx < display_components.size() &&
         largest_idx < device_components.size() &&
         display_components[largest_idx] == device_components[largest_idx]) {
    ++largest_idx;
  }
  return largest_idx;
}

// Tries to find a UDL device that best matches |display|. Returns nullptr
// if one is not found.
const ui::TouchscreenDevice* GuessBestUdlDevice(
    const ManagedDisplayInfo* display,
    const DeviceList& devices) {
  int best_score = 0;
  const ui::TouchscreenDevice* best_device = nullptr;

  for (const ui::TouchscreenDevice* device : devices) {
    int score = GetUdlAssociationScore(display, device);
    if (score > best_score) {
      best_score = score;
      best_device = device;
    }
  }

  return best_device;
}

void AssociateUdlDevices(DisplayInfoPointerList* displays,
                         DeviceList* devices) {
  VLOG(2) << "Trying to match udl devices (" << displays->size()
          << " displays and " << devices->size() << " devices to match)";

  DisplayInfoPointerList::iterator display_it = displays->begin();
  while (display_it != displays->end()) {
    ManagedDisplayInfo* display = *display_it;
    const ui::TouchscreenDevice* device = GuessBestUdlDevice(display, *devices);

    if (device) {
      VLOG(2) << "=> Matched device " << device->name << " to display "
              << display->name()
              << " (score=" << GetUdlAssociationScore(display, device) << ")";
      Associate(display, device);

      devices->erase(std::find(devices->begin(), devices->end(), device));
    }

    ++display_it;
  }
}

// Returns true if |display| is internal.
bool IsInternalDisplay(const ManagedDisplayInfo* display) {
  return Display::IsInternalDisplayId(display->id());
}

// Returns true if |device| is internal.
bool IsInternalDevice(const ui::TouchscreenDevice* device) {
  return device->type == ui::InputDeviceType::INPUT_DEVICE_INTERNAL;
}

ManagedDisplayInfo* GetInternalDisplay(DisplayInfoPointerList* displays) {
  auto it =
      std::find_if(displays->begin(), displays->end(), &IsInternalDisplay);
  if (it != displays->end())
    return *it;
  return nullptr;
}

void AssociateSingleDisplayAndSingleDevice(DisplayInfoPointerList* displays,
                                           DeviceList* devices) {
  if (displays->size() != 1 || devices->size() != 1)
    return;
  // If there is only one display and only one touch device, just associate
  // them. This fixes the issue that usb tablet input device doesn't work
  // with chromebook.
  DisplayInfoPointerList::iterator display_it = displays->begin();
  DeviceList::iterator device_it = devices->begin();
  Associate(*display_it, *device_it);
  VLOG(2) << "=> Matched single device " << (*device_it)->name
          << " to single display " << (*display_it)->name();
  devices->erase(device_it);
  return;
}

void AssociateInternalDevices(DisplayInfoPointerList* displays,
                              DeviceList* devices) {
  VLOG(2) << "Trying to match internal devices (" << displays->size()
          << " displays and " << devices->size() << " devices to match)";

  // Internal device assocation has a couple of gotchas:
  // - There can be internal devices but no internal display, or visa-versa.
  // - There can be multiple internal devices matching one internal display. We
  //   assume there is at most one internal display.
  // - All internal devices must be removed from |displays| and |devices| after
  //   this function has returned, since an internal device can never be
  //   associated with an external device.

  // Capture the internal display reference as we remove it from |displays|.
  ManagedDisplayInfo* internal_display = GetInternalDisplay(displays);

  bool matched = false;

  // Remove all internal devices from |devices|. If we have an internal display,
  // then associate the device with the display before removing it.
  DeviceList::iterator device_it = devices->begin();
  while (device_it != devices->end()) {
    const ui::TouchscreenDevice* internal_device = *device_it;

    // Not an internal device, skip it.
    if (!IsInternalDevice(internal_device)) {
      ++device_it;
      continue;
    }

    if (internal_display) {
      VLOG(2) << "=> Matched device " << internal_device->name << " to display "
              << internal_display->name();
      Associate(internal_display, internal_device);
      matched = true;
    } else {
      VLOG(2) << "=> Removing internal device " << internal_device->name;
    }

    device_it = devices->erase(device_it);
  }

  if (!matched && internal_display)
    VLOG(2) << "=> Removing internal display " << internal_display->name();
}

void AssociateFromDisplayPreferences(DisplayInfoPointerList* displays,
                                     DeviceList* devices,
                                     const DisplayManager* display_manager) {
  if (!displays->size() || !devices->size())
    return;

  VLOG(2) << "Trying to match " << displays->size() << " displays and "
          << devices->size() << " devices using display preference data.";

  std::map<uint32_t, int> touch_device_identifiers;

  // device_it is incremented within the loop.
  for (auto device_it = devices->begin(); device_it != devices->end();) {
    uint32_t touch_device_identifier =
        TouchCalibrationData::GenerateTouchDeviceIdentifier(**device_it);

    device_it = [&, touch_device_identifier]() {
      for (auto* new_display_info : *displays) {
        // During tests the display ids may be invalid.
        if (!display_manager->IsDisplayIdValid(new_display_info->id()))
          continue;

        // The existing ManagedDisplayInfo has the display preferences stored
        // with it.
        const ManagedDisplayInfo& info =
            display_manager->GetDisplayInfo(new_display_info->id());

        if (info.HasTouchDevice(touch_device_identifier)) {
          Associate(new_display_info, *device_it);
          VLOG(2) << "=> Matched device " << (*device_it)->name
                  << " to display " << new_display_info->name();
          return devices->erase(device_it);
        }
      }
      return ++device_it;
    }();
  }
}

void AssociateSameSizeDevices(DisplayInfoPointerList* displays,
                              DeviceList* devices) {
  // Associate screens/displays with the same size.
  VLOG(2) << "Trying to match same-size devices (" << displays->size()
          << " displays and " << devices->size() << " devices to match)";

  DisplayInfoPointerList::iterator display_it = displays->begin();
  while (display_it != displays->end()) {
    ManagedDisplayInfo* display = *display_it;
    const gfx::Size native_size = display->GetNativeModeSize();

    // Try to find an input device with roughly the same size as the display.
    DeviceList::iterator device_it = std::find_if(
        devices->begin(), devices->end(),
        [&native_size](const ui::TouchscreenDevice* device) {
          // Allow 1 pixel difference between screen and touchscreen
          // resolutions. Because in some cases for monitor resolution
          // 1024x768 touchscreen's resolution would be 1024x768, but for
          // some 1023x767. It really depends on touchscreen's firmware
          // configuration.
          return std::abs(native_size.width() - device->size.width()) <= 1 &&
                 std::abs(native_size.height() - device->size.height()) <= 1;
        });

    if (device_it != devices->end()) {
      const ui::TouchscreenDevice* device = *device_it;
      VLOG(2) << "=> Matched device " << device->name << " to display "
              << display->name() << " (display_size: " << native_size.ToString()
              << ", device_size: " << device->size.ToString() << ")";
      Associate(display, device);

      device_it = devices->erase(device_it);
    }

    // Didn't find an input device. Skip this display.
    ++display_it;
  }
}

void AssociateToSingleDisplay(DisplayInfoPointerList* displays,
                              DeviceList* devices) {
  // If there is only one display left, then we should associate all input
  // devices with it.

  VLOG(2) << "Trying to match to single display (" << displays->size()
          << " displays and " << devices->size() << " devices to match)";

  std::size_t num_displays_excluding_internal = displays->size();
  ManagedDisplayInfo* internal_display = GetInternalDisplay(displays);
  if (internal_display)
    num_displays_excluding_internal--;

  // We only associate to one display.
  if (num_displays_excluding_internal != 1 || devices->empty())
    return;

  // Pick the non internal display.
  ManagedDisplayInfo* display = *displays->begin();
  if (display == internal_display)
    display = (*displays)[1];

  for (const ui::TouchscreenDevice* device : *devices) {
    VLOG(2) << "=> Matched device " << device->name << " to display "
            << display->name();
    Associate(display, device);
  }
  devices->clear();
}

void AssociateRemainingDevicesToInternalOrFallbackDisplay(
    DisplayInfoPointerList* displays,
    DeviceList* devices) {
  if (!displays->size() || !devices->size())
    return;

  ManagedDisplayInfo* internal_display = GetInternalDisplay(displays);
  if (!internal_display) {
    // If no internal displays were found, then associate the devices to any of
    // the other displays.
    internal_display = *(displays->begin());

    VLOG(2) << "Could not find any internal display. Matching all devices to a "
            << "non internal display.";
  }
  VLOG(2) << "Matching " << devices->size() << " touch devices to "
          << internal_display->name() << "[" << internal_display->id() << "]";

  for (auto device_it = devices->begin(); device_it != devices->end();) {
    // We do not want to associate an internal touch device with anything other
    // than internal display.
    if ((*device_it)->type == ui::InputDeviceType::INPUT_DEVICE_INTERNAL) {
      device_it++;
      continue;
    }

    VLOG(2) << "Matching " << (*device_it)->name << " to "
            << internal_display->name();
    Associate(internal_display, *device_it);
    device_it = devices->erase(device_it);
  }
}

}  // namespace

void AssociateTouchscreens(
    std::vector<ManagedDisplayInfo>* all_displays,
    const std::vector<ui::TouchscreenDevice>& all_devices,
    const DisplayManager* display_manager) {
  // |displays| and |devices| contain pointers directly to the values stored
  // inside of |all_displays| and |all_devices|. When a display or input device
  // has been associated, it is removed from the |devices| list. When a display
  // is associated, it is *not* removed from |displays|.

  // Construct our initial set of display/devices that we will process.
  DisplayInfoPointerList displays;
  for (ManagedDisplayInfo& display : *all_displays) {
    display.ClearTouchDevices();
    displays.push_back(&display);
  }

  // Construct initial set of devices.
  DeviceList devices;
  for (const ui::TouchscreenDevice& device : all_devices)
    devices.push_back(&device);

  for (const ManagedDisplayInfo* display : displays) {
    VLOG(2) << "Received display " << display->name()
            << " (size: " << display->GetNativeModeSize().ToString()
            << ", sys_path: " << display->sys_path().LossyDisplayName() << ")";
  }
  for (const ui::TouchscreenDevice* device : devices) {
    VLOG(2) << "Received device " << device->name
            << " (size: " << device->size.ToString()
            << ", sys_path: " << device->sys_path.LossyDisplayName() << ")";
  }

  AssociateSingleDisplayAndSingleDevice(&displays, &devices);
  AssociateInternalDevices(&displays, &devices);
  AssociateFromDisplayPreferences(&displays, &devices, display_manager);
  AssociateUdlDevices(&displays, &devices);
  AssociateSameSizeDevices(&displays, &devices);
  AssociateToSingleDisplay(&displays, &devices);

  for (const ManagedDisplayInfo* display : displays)
    VLOG(2) << "Unmatched display " << display->name();
  for (const ui::TouchscreenDevice* device : devices)
    LOG(WARNING) << "Unmatched device " << device->name;

  // If there are still unmatched touch devices, associate all of them to the
  // internal display.
  if (devices.size())
    AssociateRemainingDevicesToInternalOrFallbackDisplay(&displays, &devices);
}

bool IsInternalTouchscreenDevice(uint32_t touch_device_identifier) {
  for (const auto& device :
       ui::InputDeviceManager::GetInstance()->GetTouchscreenDevices()) {
    if (device.type == ui::InputDeviceType::INPUT_DEVICE_EXTERNAL)
      continue;
    if (TouchCalibrationData::GenerateTouchDeviceIdentifier(device) ==
        touch_device_identifier) {
      return true;
    }
  }
  return false;
}

bool HasExternalTouchscreenDevice() {
  for (const auto& device :
       ui::InputDeviceManager::GetInstance()->GetTouchscreenDevices()) {
    if (device.type == ui::InputDeviceType::INPUT_DEVICE_EXTERNAL)
      return true;
  }
  return false;
}

}  // namespace display
