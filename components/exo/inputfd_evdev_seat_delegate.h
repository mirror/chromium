// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_DELEGATE_H_
#define COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_DELEGATE_H_

namespace exo {
class Surface;
class InputfdEvdevSeat;
class InputfdEvdevDeviceDelegate;

// It send device_added event and generate the InputfdEvdevDevice.
class InputfdEvdevSeatDelegate {
 public:
  // Gives the delegate a chance to clean up when the GamingSeat instance is
  // destroyed
  virtual void OnInputfdEvdevSeatDestroying(InputfdEvdevSeat* evdev_seat) = 0;

  // This should return true if |surface| is a valid gamepad events target for
  // this seat. E.g. the surface is owned by the same client as this seat.
  virtual bool CanAcceptGamepadEventsForSurface(Surface* surface) const = 0;

  // When a new evdev device is connected, inputfd_evdev seat call this to get
  // the evdev device delegate.
  virtual InputfdEvdevDeviceDelegate* DeviceAdded() = 0;

 protected:
  virtual ~InputfdEvdevSeatDelegate() {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_DELEGATE_H_
