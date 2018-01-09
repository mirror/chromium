// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_UDEV_GAMEPAD_
#define DEVICE_GAMEPAD_UDEV_GAMEPAD_

#include <memory>
#include <string>

extern "C" {
struct udev_device;
}

namespace device {

class UdevGamepad {
 public:
  enum class Type {
    JOYDEV,
    EVDEV,
    HIDRAW,
  };

  ~UdevGamepad() = default;

  // Factory method for creating UdevGamepad instances. Extracts information
  // about the device and returns a UdevGamepad describing it, or nullptr if the
  // device cannot be a gamepad.
  static std::unique_ptr<UdevGamepad> Create(udev_device* dev);

  const Type type;
  const int index;
  const std::string path;
  const std::string syspath_prefix;

 private:
  UdevGamepad(Type type,
              int index,
              const std::string& path,
              const std::string& syspath_prefix);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_UDEV_GAMEPAD_
