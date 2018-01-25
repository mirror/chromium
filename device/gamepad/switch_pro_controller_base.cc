// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/switch_pro_controller_base.h"

namespace {
const uint32_t kVendorNintendo = 0x057e;
const uint32_t kProductSwitchProController = 0x2009;
const uint8_t kRumbleMagnitudeMax = 0xff;

enum ControllerType { UNKNOWN_CONTROLLER, SWITCH_PRO_CONTROLLER };

ControllerType ControllerTypeFromDeviceIds(int vendor_id, int product_id) {
  if (vendor_id == kVendorNintendo) {
    switch (product_id) {
      case kProductSwitchProController:
        return SWITCH_PRO_CONTROLLER;
      default:
        break;
    }
  }
  return UNKNOWN_CONTROLLER;
}

}  // namespace

namespace device {

SwitchProControllerBase::~SwitchProControllerBase() = default;

// static
bool SwitchProControllerBase::IsSwitchPro(int vendor_id, int product_id) {
  return ControllerTypeFromDeviceIds(vendor_id, product_id) !=
         UNKNOWN_CONTROLLER;
}

void SwitchProControllerBase::SendConnectionStatusQuery() {
  // Requests the current connection status and info about the connected
  // controller. The controller will respond with a status packet.
  const size_t report_length = 2;
  uint8_t control_report[report_length];
  memset(control_report, 0, report_length);
  control_report[0] = 0x80;
  control_report[1] = 0x01;

  WriteOutputReport(control_report, report_length);
}

void SwitchProControllerBase::SendHandshake() {
  // Sends handshaking packets over UART. This command can only be called once
  // per session. The controller will respond with a status packet.
  const size_t report_length = 64;
  uint8_t control_report[report_length];
  memset(control_report, 0, report_length);
  control_report[0] = 0x80;
  control_report[1] = 0x02;

  WriteOutputReport(control_report, report_length);
}

void SwitchProControllerBase::SendForceUsbHid(bool enable) {
  // By default, the controller will revert to Bluetooth mode if it does not
  // receive any USB HID commands within a timeout window. Enabling the
  // ForceUsbHid mode forces all communication to go through USB HID and
  // disables the timeout.
  const size_t report_length = 64;
  uint8_t control_report[report_length];
  memset(control_report, 0, report_length);
  control_report[0] = 0x80;
  control_report[1] = (enable ? 0x04 : 0x05);

  WriteOutputReport(control_report, report_length);
}

void SwitchProControllerBase::SetVibration(double strong_magnitude,
                                           double weak_magnitude) {
  uint8_t strong_magnitude_scaled =
      static_cast<uint8_t>(strong_magnitude * kRumbleMagnitudeMax);
  uint8_t weak_magnitude_scaled =
      static_cast<uint8_t>(weak_magnitude * kRumbleMagnitudeMax);

  const size_t report_length = 64;
  uint8_t control_report[report_length];
  memset(control_report, 0, report_length);
  control_report[0] = 0x10;
  control_report[1] = static_cast<uint8_t>(counter_++ & 0x0F);
  control_report[2] = 0x80;
  control_report[6] = 0x80;
  if (strong_magnitude_scaled > 0) {
    control_report[2] = 0x80;
    control_report[3] = 0x20;
    control_report[4] = 0x62;
    control_report[5] = strong_magnitude_scaled >> 2;
  }
  if (weak_magnitude_scaled > 0) {
    control_report[6] = 0x98;
    control_report[7] = 0x20;
    control_report[8] = 0x62;
    control_report[9] = weak_magnitude_scaled >> 2;
  }

  WriteOutputReport(control_report, report_length);
}

}  // namespace device
