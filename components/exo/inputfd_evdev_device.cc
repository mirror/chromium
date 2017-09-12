// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/inputfd_evdev_device.h"
#include <fcntl.h>
#include <linux/input.h>

#include "components/exo/inputfd_evdev_device_delegate.h"
#include "components/exo/surface.h"
#include "ui/events/devices/input_device.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// InputfdEvdevDevice, public:
InputfdEvdevDevice::InputfdEvdevDevice(
    const ui::InputDevice& device,
    InputfdEvdevDeviceDelegate* evdev_device_delegate)
    : serial_(0),
      sys_path_(device.sys_path),
      fd_(open(sys_path_.value().c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK)),
      evdev_device_delegate_(evdev_device_delegate) {
  evdev_device_delegate_->OnInitName(device.name.c_str());
  evdev_device_delegate_->OnInitUsbID(device.vendor_id, device.product_id);
  evdev_device_delegate_->OnInitDone();
}

InputfdEvdevDevice::~InputfdEvdevDevice() {
  evdev_device_delegate_->OnRemoved();
}

void InputfdEvdevDevice::OnFocusIn(Surface* surface) {
  fd_.reset(open(sys_path_.value().c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK));
  evdev_device_delegate_->OnFocusIn(serial_++, fd_.get(), surface);
}

void InputfdEvdevDevice::OnFocusOut() {
  evdev_device_delegate_->OnFocusOut();
  ioctl(fd_.get(), EVIOCREVOKE, NULL);
  fd_.reset();
}

}  // namespace exo
