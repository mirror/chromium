// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "raw_input_gamepad_device_win.h"

namespace device {

namespace {

float NormalizeAxis(long value, long min, long max) {
  return (2.f * (value - min) / static_cast<float>(max - min)) - 1.f;
}

unsigned long GetBitmask(unsigned short bits) {
  return (1 << bits) - 1;
}

const uint32_t kAxisMinimumUsageNumber = 0x30;
const uint32_t kGameControlsUsagePage = 0x05;
const uint32_t kButtonUsagePage = 0x09;

// Blacklisted vendor IDs.
const uint32_t kVendorOculus = 0x2833;
const uint32_t kVendorBlue = 0xb58e;

}  // namespace

RawInputGamepadDeviceWin::RawInputGamepadDeviceWin(
    HANDLE device_handle,
    int source_id,
    HidDllFunctionsWin* hid_functions)
    : handle_(device_handle),
      source_id_(source_id),
      hid_functions_(hid_functions) {
  ::ZeroMemory(buttons_, sizeof(buttons_));
  ::ZeroMemory(axes_, sizeof(axes_));

  if (hid_functions_->IsValid())
    is_valid_ = QueryDeviceInfo();
}

RawInputGamepadDeviceWin::~RawInputGamepadDeviceWin() = default;

// static
bool RawInputGamepadDeviceWin::IsGamepadUsageId(uint16_t usage) {
  return usage == kGenericDesktopJoystick || usage == kGenericDesktopGamePad ||
         usage == kGenericDesktopMultiAxisController;
}

void RawInputGamepadDeviceWin::UpdateGamepad(RAWINPUT* input) {
  DCHECK(hid_functions_->IsValid());
  NTSTATUS status;

  report_id_++;

  // Query button state.
  if (buttons_length_ > 0) {
    // Clear the button state
    ::ZeroMemory(buttons_, sizeof(buttons_));
    ULONG buttons_length = 0;

    hid_functions_->HidPGetUsagesEx()(
        HidP_Input, 0, nullptr, &buttons_length, preparsed_data_,
        reinterpret_cast<PCHAR>(input->data.hid.bRawData),
        input->data.hid.dwSizeHid);

    std::unique_ptr<USAGE_AND_PAGE[]> usages(
        new USAGE_AND_PAGE[buttons_length]);
    status = hid_functions_->HidPGetUsagesEx()(
        HidP_Input, 0, usages.get(), &buttons_length, preparsed_data_,
        reinterpret_cast<PCHAR>(input->data.hid.bRawData),
        input->data.hid.dwSizeHid);

    if (status == HIDP_STATUS_SUCCESS) {
      // Set each reported button to true.
      for (uint32_t j = 0; j < buttons_length; j++) {
        int32_t button_index = usages[j].Usage - 1;
        if (usages[j].UsagePage == kButtonUsagePage && button_index >= 0 &&
            button_index < static_cast<int>(Gamepad::kButtonsLengthCap)) {
          buttons_[button_index] = true;
        }
      }
    }
  }

  // Query axis state.
  ULONG axis_value = 0;
  LONG scaled_axis_value = 0;
  for (uint32_t i = 0; i < axes_length_; i++) {
    RawGamepadAxis* axis = &axes_[i];

    // If the min is < 0 we have to query the scaled value, otherwise we need
    // the normal unscaled value.
    if (axis->caps.LogicalMin < 0) {
      status = hid_functions_->HidPGetScaledUsageValue()(
          HidP_Input, axis->caps.UsagePage, 0, axis->caps.Range.UsageMin,
          &scaled_axis_value, preparsed_data_,
          reinterpret_cast<PCHAR>(input->data.hid.bRawData),
          input->data.hid.dwSizeHid);
      if (status == HIDP_STATUS_SUCCESS) {
        axis->value = NormalizeAxis(scaled_axis_value, axis->caps.PhysicalMin,
                                    axis->caps.PhysicalMax);
      }
    } else {
      status = hid_functions_->HidPGetUsageValue()(
          HidP_Input, axis->caps.UsagePage, 0, axis->caps.Range.UsageMin,
          &axis_value, preparsed_data_,
          reinterpret_cast<PCHAR>(input->data.hid.bRawData),
          input->data.hid.dwSizeHid);
      if (status == HIDP_STATUS_SUCCESS) {
        axis->value = NormalizeAxis(axis_value & axis->bitmask,
                                    axis->caps.LogicalMin & axis->bitmask,
                                    axis->caps.LogicalMax & axis->bitmask);
      }
    }
  }
}

void RawInputGamepadDeviceWin::ReadPadState(Gamepad* pad) const {
  DCHECK(pad);

  pad->timestamp = report_id_;
  pad->buttons_length = buttons_length_;
  pad->axes_length = axes_length_;

  for (unsigned int i = 0; i < buttons_length_; i++) {
    pad->buttons[i].pressed = buttons_[i];
    pad->buttons[i].value = buttons_[i] ? 1.0 : 0.0;
  }

  for (unsigned int i = 0; i < axes_length_; i++)
    pad->axes[i] = axes_[i].value;
}

bool RawInputGamepadDeviceWin::QueryDeviceInfo() {
  // Fetch HID properties (RID_DEVICE_INFO_HID) for this device. This includes
  // |vendor_id_|, |product_id_|, |version_number_|, and |usage_|.
  if (!QueryHidInfo())
    return false;

  // Make sure this device is of a type that we want to observe.
  if (!IsGamepadUsageId(usage_))
    return false;

  // This is terrible, but the Oculus Rift and some Blue Yeti microphones seem
  // to think they are gamepads. Filter out any such devices. Oculus Touch is
  // handled elsewhere.
  if (vendor_id_ == kVendorOculus || vendor_id_ == kVendorBlue)
    return false;

  // Fetch the device's |name_| (RIDI_DEVICENAME).
  if (!QueryDeviceName())
    return false;

  // Fetch the human-friendly |product_string_|, if available.
  if (!QueryProductString())
    product_string_ = L"Unknown Gamepad";

  // Fetch information about the buttons and axes on this device. This sets
  // |buttons_length_| and |axes_length_| to their correct values and populates
  // |axes_| with capabilities info.
  if (!QueryDeviceCapabilities())
    return false;

  // Gamepads must have at least one button or axis.
  if (buttons_length_ == 0 && axes_length_ == 0)
    return false;

  return true;
}

bool RawInputGamepadDeviceWin::QueryHidInfo() {
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(0u, result);

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
  result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICEINFO, buffer.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(size, result);
  RID_DEVICE_INFO* device_info =
      reinterpret_cast<RID_DEVICE_INFO*>(buffer.get());

  DCHECK_EQ(device_info->dwType, static_cast<DWORD>(RIM_TYPEHID));
  vendor_id_ = device_info->hid.dwVendorId;
  product_id_ = device_info->hid.dwProductId;
  version_number_ = device_info->hid.dwVersionNumber;
  usage_ = device_info->hid.usUsage;

  return true;
}

bool RawInputGamepadDeviceWin::QueryDeviceName() {
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICENAME, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(0u, result);

  std::unique_ptr<wchar_t[]> buffer(new wchar_t[size]);
  result =
      ::GetRawInputDeviceInfo(handle_, RIDI_DEVICENAME, buffer.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(size, result);

  name_ = buffer.get();

  return true;
}

bool RawInputGamepadDeviceWin::QueryProductString() {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  base::win::ScopedHandle hid_handle(::CreateFile(
      name_.c_str(), GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL));
  if (!hid_handle.IsValid())
    return false;
  product_string_.resize(Gamepad::kIdLengthCap);
  return hid_functions_->HidDGetProductString()(
      hid_handle.Get(), &product_string_.front(), Gamepad::kIdLengthCap);
}

bool RawInputGamepadDeviceWin::QueryDeviceCapabilities() {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  UINT size = 0;

  UINT result =
      ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA, nullptr, &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(0u, result);

  ppd_buffer_.reset(new uint8_t[size]);
  preparsed_data_ = reinterpret_cast<PHIDP_PREPARSED_DATA>(ppd_buffer_.get());
  result = ::GetRawInputDeviceInfo(handle_, RIDI_PREPARSEDDATA,
                                   ppd_buffer_.get(), &size);
  if (result == static_cast<UINT>(-1)) {
    PLOG(ERROR) << "GetRawInputDeviceInfo() failed";
    return false;
  }
  DCHECK_EQ(size, result);

  HIDP_CAPS caps;
  NTSTATUS status = hid_functions_->HidPGetCaps()(preparsed_data_, &caps);
  DCHECK_EQ(HIDP_STATUS_SUCCESS, status);

  QueryButtonCapabilities(caps.NumberInputButtonCaps);
  QueryAxisCapabilities(caps.NumberInputValueCaps);

  return true;
}

void RawInputGamepadDeviceWin::QueryButtonCapabilities(uint16_t button_count) {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  if (button_count > 0) {
    std::unique_ptr<HIDP_BUTTON_CAPS[]> button_caps(
        new HIDP_BUTTON_CAPS[button_count]);
    NTSTATUS status = hid_functions_->HidPGetButtonCaps()(
        HidP_Input, button_caps.get(), &button_count, preparsed_data_);
    DCHECK_EQ(HIDP_STATUS_SUCCESS, status);

    for (size_t i = 0; i < button_count; ++i) {
      if (button_caps[i].Range.UsageMin <= Gamepad::kButtonsLengthCap &&
          button_caps[i].UsagePage == kButtonUsagePage) {
        size_t max_index =
            std::min(Gamepad::kButtonsLengthCap,
                     static_cast<size_t>(button_caps[i].Range.UsageMax));
        buttons_length_ = std::max(buttons_length_, max_index);
      }
    }
  }
}

void RawInputGamepadDeviceWin::QueryAxisCapabilities(uint16_t axis_count) {
  DCHECK(hid_functions_);
  DCHECK(hid_functions_->IsValid());
  std::unique_ptr<HIDP_VALUE_CAPS[]> axes_caps(new HIDP_VALUE_CAPS[axis_count]);
  hid_functions_->HidPGetValueCaps()(HidP_Input, axes_caps.get(), &axis_count,
                                     preparsed_data_);

  bool mapped_all_axes = true;

  for (size_t i = 0; i < axis_count; i++) {
    size_t axis_index = axes_caps[i].Range.UsageMin - kAxisMinimumUsageNumber;
    if (axis_index < Gamepad::kAxesLengthCap) {
      axes_[axis_index].caps = axes_caps[i];
      axes_[axis_index].value = 0;
      axes_[axis_index].active = true;
      axes_[axis_index].bitmask = GetBitmask(axes_caps[i].BitSize);
      axes_length_ = std::max(axes_length_, axis_index + 1);
    } else {
      mapped_all_axes = false;
    }
  }

  if (!mapped_all_axes) {
    // For axes whose usage puts them outside the standard axesLengthCap range.
    size_t next_index = 0;
    for (size_t i = 0; i < axis_count; i++) {
      size_t usage = axes_caps[i].Range.UsageMin - kAxisMinimumUsageNumber;
      if (usage >= Gamepad::kAxesLengthCap &&
          axes_caps[i].UsagePage <= kGameControlsUsagePage) {
        for (; next_index < Gamepad::kAxesLengthCap; ++next_index) {
          if (!axes_[next_index].active)
            break;
        }
        if (next_index < Gamepad::kAxesLengthCap) {
          axes_[next_index].caps = axes_caps[i];
          axes_[next_index].value = 0;
          axes_[next_index].active = true;
          axes_[next_index].bitmask = GetBitmask(axes_caps[i].BitSize);
          axes_length_ = std::max(axes_length_, next_index + 1);
        }
      }

      if (next_index >= Gamepad::kAxesLengthCap)
        break;
    }
  }
}

}  // namespace device
