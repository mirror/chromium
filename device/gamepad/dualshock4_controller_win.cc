// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/dualshock4_controller_win.h"

namespace {
const uint32_t kVendorSony = 0x054c;
const uint32_t kProductDualshock4 = 0x05c4;
const uint32_t kProductDualshock4Slim = 0x9cc;
const uint8_t kRumbleMagnitudeMax = 0xff;

base::win::ScopedHandle OpenHidHandle(HANDLE device_handle) {
  base::win::ScopedHandle hid_handle;
  UINT size;
  UINT result =
      ::GetRawInputDeviceInfo(device_handle, RIDI_DEVICENAME, nullptr, &size);
  if (result == 0U) {
    std::unique_ptr<wchar_t[]> name_buffer(new wchar_t[size]);
    result = ::GetRawInputDeviceInfo(device_handle, RIDI_DEVICENAME,
                                     name_buffer.get(), &size);
    if (result == size) {
      hid_handle.Set(::CreateFile(name_buffer.get(),
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, 0U, nullptr));
    }
  }
  return hid_handle;
}

bool WriteControlReport(HANDLE hid_handle, void* report, DWORD report_length) {
  DWORD bytes_written;
  BOOL res =
      ::WriteFile(hid_handle, report, report_length, &bytes_written, nullptr);
  return res && bytes_written == report_length;
}

}  // namespace

namespace device {

Dualshock4ControllerWin::Dualshock4ControllerWin(HANDLE device_handle)
    : sequence_id_(0), hid_handle_(OpenHidHandle(device_handle)) {}

Dualshock4ControllerWin::~Dualshock4ControllerWin() {
  if (pending_callback_) {
    std::move(pending_callback_)
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }
}

// static
Dualshock4ControllerWin::ControllerType
Dualshock4ControllerWin::ControllerTypeFromDeviceIds(int vendor_id,
                                                     int product_id) {
  if (vendor_id == kVendorSony) {
    switch (product_id) {
      case kProductDualshock4:
        return DUALSHOCK4_CONTROLLER;
      case kProductDualshock4Slim:
        return DUALSHOCK4_SLIM_CONTROLLER;
      default:
        break;
    }
  }
  return UNKNOWN_CONTROLLER;
}

// static
bool Dualshock4ControllerWin::IsDualshock4(int vendor_id, int product_id) {
  return ControllerTypeFromDeviceIds(vendor_id, product_id) !=
         UNKNOWN_CONTROLLER;
}

void Dualshock4ControllerWin::PlayEffect(
    mojom::GamepadHapticEffectType type,
    mojom::GamepadEffectParametersPtr params,
    mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
  if (type !=
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble) {
    // Only dual-rumble effects are supported.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  if (!hid_handle_.IsValid()) {
    // Failed to open HID device.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  sequence_id_++;

  if (pending_callback_) {
    std::move(pending_callback_)
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }
  pending_callback_ = std::move(callback);

  PlayDualRumbleEffect(params->duration, params->start_delay,
                       params->strong_magnitude, params->weak_magnitude);
}

void Dualshock4ControllerWin::ResetVibration(
    mojom::GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
  if (!hid_handle_.IsValid()) {
    // Failed to open HID device.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  sequence_id_++;

  if (pending_callback_) {
    SetVibration(0.0, 0.0);
    std::move(pending_callback_)
        .Run(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }

  std::move(callback).Run(
      mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
}

void Dualshock4ControllerWin::PlayDualRumbleEffect(double duration,
                                                   double start_delay,
                                                   double strong_magnitude,
                                                   double weak_magnitude) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&Dualshock4ControllerWin::StartVibration,
                     base::Unretained(this), sequence_id_, duration,
                     strong_magnitude, weak_magnitude),
      base::TimeDelta::FromMillisecondsD(start_delay));
}

void Dualshock4ControllerWin::StartVibration(int sequence_id,
                                             double duration,
                                             double strong_magnitude,
                                             double weak_magnitude) {
  if (sequence_id != sequence_id_)
    return;
  SetVibration(strong_magnitude, weak_magnitude);

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&Dualshock4ControllerWin::StopVibration,
                     base::Unretained(this), sequence_id),
      base::TimeDelta::FromMillisecondsD(duration));
}

void Dualshock4ControllerWin::StopVibration(int sequence_id) {
  if (sequence_id != sequence_id_)
    return;
  SetVibration(0.0, 0.0);

  std::move(pending_callback_)
      .Run(mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
}

void Dualshock4ControllerWin::SetVibration(double strong_magnitude,
                                           double weak_magnitude) {
  DCHECK(hid_handle_.IsValid());

  const size_t report_length = 32;
  uint8_t control_report[report_length];
  ::ZeroMemory(control_report, report_length);
  control_report[0] = 0x05;  // report ID
  control_report[1] = 0x01;  // motor only, don't update LEDs
  control_report[4] =
      static_cast<uint8_t>(weak_magnitude * kRumbleMagnitudeMax);
  control_report[5] =
      static_cast<uint8_t>(strong_magnitude * kRumbleMagnitudeMax);

  bool res =
      WriteControlReport(hid_handle_.Get(), control_report, report_length);
  DCHECK(res);
}

}  // namespace device
