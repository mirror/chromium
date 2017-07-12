// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_KEYBOARD_DEVICE_CONFIGURATION_H_
#define COMPONENTS_EXO_KEYBOARD_DEVICE_CONFIGURATION_H_

#include "components/exo/keyboard_observer.h"

namespace exo {

// Used as an extension to the KeyboardDelegate.
class KeyboardDeviceConfigurationDelegate : public KeyboardObserver {
 public:
  // Called when used keyboard type changed.
  virtual void OnKeyboardTypeChanged(bool is_physical) = 0;

 protected:
  ~KeyboardDeviceConfigurationDelegate() override {}
};

}  // namespace exo

#endif  // COMPONENTS_EXO_KEYBOARD_DEVICE_CONFIGURATION_H_
