// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/switch_pro_controller_linux.h"

#include "base/posix/eintr_wrapper.h"
#include "device/gamepad/gamepad_standard_mappings.h"

namespace device {

namespace {

double NormalizeAndClampAxis(int value, int min, int max) {
  if (value <= min)
    return -1.0;
  if (value >= max)
    return 1.0;
  return (2.0 * (value - min) / static_cast<double>(max - min)) - 1.0;
}

#pragma pack(push, 1)
struct SwitchUsbControllerDataReport {
  uint8_t type;  // must be kPacketTypeControllerData
  uint8_t timestamp;
  uint8_t dummy1;
  bool button_y : 1;
  bool button_x : 1;
  bool button_b : 1;
  bool button_a : 1;
  bool dummy2 : 2;
  bool button_r : 1;
  bool button_zr : 1;
  bool button_minus : 1;
  bool button_plus : 1;
  bool button_thumb_r : 1;
  bool button_thumb_l : 1;
  bool button_home : 1;
  bool button_capture : 1;
  bool dummy3 : 2;
  bool dpad_down : 1;
  bool dpad_up : 1;
  bool dpad_right : 1;
  bool dpad_left : 1;
  bool dummy4 : 2;
  bool button_l : 1;
  bool button_zl : 1;
  uint8_t analog[6];
};
#pragma pack(pop)

// Switch Pro Controller packet types.
static const uint8_t kPacketTypeStatus = 0x81;
static const uint8_t kPacketTypeControllerData = 0x30;

// Status packet constants.
static const uint8_t kStatusTypeSerial = 0x01;
static const uint8_t kStatusTypeInit = 0x02;

// Axis extents, used for normalization.
static const int kAxisMin = -128;
static const int kAxisMax = 127;
}  // namespace

SwitchProControllerLinux::SwitchProControllerLinux(int fd) : fd_(fd) {}

SwitchProControllerLinux::~SwitchProControllerLinux() = default;

void SwitchProControllerLinux::DoShutdown() {
  if (force_usb_hid_)
    SendForceUsbHid(false);
  force_usb_hid_ = false;
}

void SwitchProControllerLinux::ReadUsbPadState(Gamepad* pad) {
  DCHECK_GE(fd_, 0);

  uint8_t buf[64];
  ssize_t len = 0;
  while ((len = HANDLE_EINTR(read(fd_, buf, 64))) > 0) {
    uint8_t type = buf[0];
    switch (type) {
      case kPacketTypeStatus: {
        uint8_t status_type = buf[1];
        switch (status_type) {
          case kStatusTypeSerial:
            if (!sent_handshake_) {
              sent_handshake_ = true;
              SendHandshake();
            }
            break;
          case kStatusTypeInit:
            SendForceUsbHid(true);
            force_usb_hid_ = true;
            break;
        }
        break;
      }
      case kPacketTypeControllerData: {
        SwitchUsbControllerDataReport* report =
            reinterpret_cast<SwitchUsbControllerDataReport*>(buf);
        pad->buttons[BUTTON_INDEX_PRIMARY].pressed = report->button_b;
        pad->buttons[BUTTON_INDEX_PRIMARY].value = report->button_b ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_SECONDARY].pressed = report->button_a;
        pad->buttons[BUTTON_INDEX_SECONDARY].value =
            report->button_a ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_TERTIARY].pressed = report->button_y;
        pad->buttons[BUTTON_INDEX_TERTIARY].value =
            report->button_y ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_QUATERNARY].pressed = report->button_x;
        pad->buttons[BUTTON_INDEX_QUATERNARY].value =
            report->button_x ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_LEFT_SHOULDER].pressed = report->button_l;
        pad->buttons[BUTTON_INDEX_LEFT_SHOULDER].value =
            report->button_l ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_RIGHT_SHOULDER].pressed = report->button_r;
        pad->buttons[BUTTON_INDEX_RIGHT_SHOULDER].value =
            report->button_r ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_LEFT_TRIGGER].pressed = report->button_zl;
        pad->buttons[BUTTON_INDEX_LEFT_TRIGGER].value =
            report->button_zl ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_RIGHT_TRIGGER].pressed = report->button_zr;
        pad->buttons[BUTTON_INDEX_RIGHT_TRIGGER].value =
            report->button_zr ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_BACK_SELECT].pressed = report->button_minus;
        pad->buttons[BUTTON_INDEX_BACK_SELECT].value =
            report->button_minus ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_START].pressed = report->button_plus;
        pad->buttons[BUTTON_INDEX_START].value =
            report->button_plus ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_LEFT_THUMBSTICK].pressed =
            report->button_thumb_l;
        pad->buttons[BUTTON_INDEX_LEFT_THUMBSTICK].value =
            report->button_thumb_l ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_RIGHT_THUMBSTICK].pressed =
            report->button_thumb_r;
        pad->buttons[BUTTON_INDEX_RIGHT_THUMBSTICK].value =
            report->button_thumb_r ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_DPAD_UP].pressed = report->dpad_up;
        pad->buttons[BUTTON_INDEX_DPAD_UP].value = report->dpad_up ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_DPAD_DOWN].pressed = report->dpad_down;
        pad->buttons[BUTTON_INDEX_DPAD_DOWN].value =
            report->dpad_down ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_DPAD_LEFT].pressed = report->dpad_left;
        pad->buttons[BUTTON_INDEX_DPAD_LEFT].value =
            report->dpad_left ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_DPAD_RIGHT].pressed = report->dpad_right;
        pad->buttons[BUTTON_INDEX_DPAD_RIGHT].value =
            report->dpad_right ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_META].pressed = report->button_home;
        pad->buttons[BUTTON_INDEX_META].value = report->button_home ? 1.0 : 0.0;

        pad->buttons[BUTTON_INDEX_META + 1].pressed = report->button_capture;
        pad->buttons[BUTTON_INDEX_META + 1].value =
            report->button_capture ? 1.0 : 0.0;

        int8_t axis_lx = (((report->analog[1] & 0x0F) << 4) |
                          ((report->analog[0] & 0xF0) >> 4)) +
                         127;
        int8_t axis_ly = report->analog[2] + 127;
        int8_t axis_rx = (((report->analog[4] & 0x0F) << 4) |
                          ((report->analog[3] & 0xF0) >> 4)) +
                         127;
        int8_t axis_ry = report->analog[5] + 127;
        pad->axes[AXIS_INDEX_LEFT_STICK_X] =
            NormalizeAndClampAxis(axis_lx, kAxisMin, kAxisMax);
        pad->axes[AXIS_INDEX_LEFT_STICK_Y] =
            NormalizeAndClampAxis(-axis_ly, kAxisMin, kAxisMax);
        pad->axes[AXIS_INDEX_RIGHT_STICK_X] =
            NormalizeAndClampAxis(axis_rx, kAxisMin, kAxisMax);
        pad->axes[AXIS_INDEX_RIGHT_STICK_Y] =
            NormalizeAndClampAxis(-axis_ry, kAxisMin, kAxisMax);

        pad->buttons_length = BUTTON_INDEX_COUNT + 1;
        pad->axes_length = AXIS_INDEX_COUNT;

        pad->timestamp = ++report_id_;
        break;
      }
      default:
        break;
    }
  }
}

void SwitchProControllerLinux::WriteOutputReport(void* report,
                                                 size_t report_length) {
  write(fd_, report, report_length);
}

}  // namespace device
