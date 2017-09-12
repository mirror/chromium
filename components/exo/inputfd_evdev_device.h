// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_H_
#define COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_H_

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"

namespace ui {
struct InputDevice;
}

namespace exo {
class InputfdEvdevDeviceDelegate;
class Surface;

// Handles device init and focus change.
class InputfdEvdevDevice {
 public:
  // Construct InputfdEvdevDevice, it will send out the inputfd device init
  // events.
  InputfdEvdevDevice(const ui::InputDevice& device,
                     InputfdEvdevDeviceDelegate* evdev_device_delegate);

  // Destruct inputfd evdev device. It will first focus out.
  ~InputfdEvdevDevice();

  void OnFocusIn(Surface* surface);
  void OnFocusOut();

 private:
  // Current serial number for focus in event.
  int serial_;

  // Sys path for this device.
  base::FilePath sys_path_;

  // Current fd sent through focus in.
  base::ScopedFD fd_;

  // Delegate to dispatch events to clients.
  InputfdEvdevDeviceDelegate* evdev_device_delegate_;

  DISALLOW_COPY_AND_ASSIGN(InputfdEvdevDevice);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_H_
