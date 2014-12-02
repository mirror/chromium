// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_INPUT_CONTROLLER_EVDEV_H_
#define UI_EVENTS_OZONE_EVDEV_INPUT_CONTROLLER_EVDEV_H_

#include <string>

#include "base/basictypes.h"
#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/ozone/public/input_controller.h"

namespace ui {

class EventFactoryEvdev;
class MouseButtonMapEvdev;

#if defined(USE_EVDEV_GESTURES)
class GesturePropertyProvider;
#endif

// Ozone InputController implementation for the Linux input subsystem ("evdev").
class EVENTS_OZONE_EVDEV_EXPORT InputControllerEvdev : public InputController {
 public:
  InputControllerEvdev(EventFactoryEvdev* event_factory,
                       MouseButtonMapEvdev* button_map
#if defined(USE_EVDEV_GESTURES)
                       ,
                       GesturePropertyProvider* gesture_property_provider
#endif
                       );
  virtual ~InputControllerEvdev();

  // InputController overrides:
  //
  // Interfaces for configuring input devices.
  bool HasMouse() override;
  bool HasTouchpad() override;
  void SetTouchpadSensitivity(int value) override;
  void SetTapToClick(bool enabled) override;
  void SetThreeFingerClick(bool enabled) override;
  void SetTapDragging(bool enabled) override;
  void SetNaturalScroll(bool enabled) override;
  void SetMouseSensitivity(int value) override;
  void SetPrimaryButtonRight(bool right) override;

 private:
  // Set a property value for all devices of one type.
  void SetIntPropertyForOneType(const EventDeviceType type,
                                const std::string& name,
                                int value);
  void SetBoolPropertyForOneType(const EventDeviceType type,
                                 const std::string& name,
                                 bool value);

  // Event factory object which manages device event converters.
  EventFactoryEvdev* event_factory_;  // Not owned.

  // Mouse button map.
  MouseButtonMapEvdev* button_map_;  // Not owned.

#if defined(USE_EVDEV_GESTURES)
  // Gesture library property provider.
  GesturePropertyProvider* gesture_property_provider_;  // Not owned.
#endif

  DISALLOW_COPY_AND_ASSIGN(InputControllerEvdev);
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_INPUT_CONTROLLER_EVDEV_H_
