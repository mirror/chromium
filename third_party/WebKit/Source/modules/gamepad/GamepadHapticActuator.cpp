// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/gamepad/GamepadHapticActuator.h"

namespace blink {

GamepadHapticActuator* GamepadHapticActuator::Create() {
  return new GamepadHapticActuator();
}

GamepadHapticActuator::GamepadHapticActuator() {}

void GamepadHapticActuator::SetType(
    const device::GamepadHapticActuatorType& type) {
  switch (type) {
    case device::GamepadHapticActuatorType::kVibration:
      type_ = "vibration";
      break;
    default:
      NOTREACHED();
  }
}

ScriptPromise GamepadHapticActuator::pulse(ScriptState* script_state,
                                           double value,
                                           double duration) {
  return ScriptPromise::Cast(script_state,
                             ScriptValue::From(script_state, false));
}

}  // namespace blink
