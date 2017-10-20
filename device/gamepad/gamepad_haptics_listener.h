// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_GAMEPAD_HAPTICS_LISTENER_H_
#define DEVICE_GAMEPAD_GAMEPAD_HAPTICS_LISTENER_H_

#include "base/macros.h"
#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/public/interfaces/gamepad.mojom.h"

namespace device {

class DEVICE_GAMEPAD_EXPORT GamepadHapticsListener
    : public mojom::GamepadHapticsListener {
 public:
  GamepadHapticsListener();
  ~GamepadHapticsListener() override;

  static void Create(mojom::GamepadHapticsListenerRequest request);

  // mojom::GamepadHapticsListener implementation.
  void PlayVibrationEffectOnce(int pad_index,
                               GamepadHapticEffectType,
                               const GamepadEffectParameters&,
                               PlayVibrationEffectOnceCallback) override;
  void ResetVibrationActuator(int pad_index,
                              ResetVibrationActuatorCallback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GamepadHapticsListener);
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_GAMEPAD_HAPTICS_LISTENER_H_
