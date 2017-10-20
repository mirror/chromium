// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_haptics_listener.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

GamepadHapticsListener::GamepadHapticsListener() = default;

GamepadHapticsListener::~GamepadHapticsListener() = default;

// static
void GamepadHapticsListener::Create(
    mojom::GamepadHapticsListenerRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<GamepadHapticsListener>(),
                          std::move(request));
}

void GamepadHapticsListener::PlayVibrationEffectOnce(
    int pad_index,
    GamepadHapticEffectType type,
    const GamepadEffectParameters& params,
    PlayVibrationEffectOnceCallback callback) {
  NOTIMPLEMENTED();
}

void GamepadHapticsListener::ResetVibrationActuator(
    int pad_index,
    ResetVibrationActuatorCallback callback) {
  NOTIMPLEMENTED();
}

}  // namespace device
