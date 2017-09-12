// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_DELEGATE_H_
#define COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_DELEGATE_H_

namespace exo {
class Surface;

// Handles events for a specific inputfd evdev device.
class InputfdEvdevDeviceDelegate {
 public:
  //////////////////////////////////////////////////////////////////////////////
  // Device init sequence:
  virtual void OnInitName(const char* name) = 0;

  virtual void OnInitUsbID(uint32_t vid, uint32_t pid) = 0;

  virtual void OnInitDone() = 0;

  //////////////////////////////////////////////////////////////////////////////
  // Focus manage:

  virtual void OnFocusIn(uint32_t serial, int32_t fd, Surface* surface) = 0;

  virtual void OnFocusOut() = 0;

  virtual void OnRemoved() = 0;

 protected:
  virtual ~InputfdEvdevDeviceDelegate() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_INPUTFD_EVDEV_DEVICE_DELEGATE_H_
