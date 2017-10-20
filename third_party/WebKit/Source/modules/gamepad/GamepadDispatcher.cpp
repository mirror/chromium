// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/gamepad/GamepadDispatcher.h"

#include "modules/gamepad/NavigatorGamepad.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/public/platform/InterfaceProvider.h"

namespace {

device::mojom::blink::GamepadHapticEffectType EffectTypeToMojoEffectType(
    device::GamepadHapticEffectType type) {
  switch (type) {
    case device::GamepadHapticEffectType::kDualRumble:
      return device::mojom::blink::GamepadHapticEffectType::
          GamepadHapticEffectTypeDualRumble;
    default:
      break;
  }

  NOTREACHED();
  return device::mojom::blink::GamepadHapticEffectType::
      GamepadHapticEffectTypeDualRumble;
}

}  // namespace

namespace blink {

using device::mojom::blink::GamepadHapticsListener;

GamepadDispatcher& GamepadDispatcher::Instance() {
  DEFINE_STATIC_LOCAL(GamepadDispatcher, gamepad_dispatcher,
                      (new GamepadDispatcher));
  return gamepad_dispatcher;
}

void GamepadDispatcher::SampleGamepads(device::Gamepads& gamepads) {
  Platform::Current()->SampleGamepads(gamepads);
}

void GamepadDispatcher::PlayVibrationEffectOnce(
    int pad_index,
    device::GamepadHapticEffectType type,
    const device::GamepadEffectParameters& params,
    GamepadHapticsListener::PlayVibrationEffectOnceCallback callback) {
  if (!gamepad_haptics_listener_) {
    std::move(callback).Run(
        device::mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }
  gamepad_haptics_listener_->PlayVibrationEffectOnce(
      pad_index, EffectTypeToMojoEffectType(type),
      device::mojom::blink::GamepadEffectParameters::New(
          params.duration, params.start_delay, params.strong_magnitude,
          params.weak_magnitude),
      std::move(callback));
}

void GamepadDispatcher::ResetVibrationActuator(
    int pad_index,
    GamepadHapticsListener::ResetVibrationActuatorCallback callback) {
  if (!gamepad_haptics_listener_) {
    std::move(callback).Run(
        device::mojom::GamepadHapticsResult::GamepadHapticsResultError);
    return;
  }
  gamepad_haptics_listener_->ResetVibrationActuator(pad_index,
                                                    std::move(callback));
}

GamepadDispatcher::GamepadDispatcher() {
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&gamepad_haptics_listener_));
}

GamepadDispatcher::~GamepadDispatcher() = default;

void GamepadDispatcher::Trace(blink::Visitor* visitor) {
  PlatformEventDispatcher::Trace(visitor);
}

void GamepadDispatcher::DidConnectGamepad(unsigned index,
                                          const device::Gamepad& gamepad) {
  DispatchDidConnectOrDisconnectGamepad(index, gamepad, true);
}

void GamepadDispatcher::DidDisconnectGamepad(unsigned index,
                                             const device::Gamepad& gamepad) {
  DispatchDidConnectOrDisconnectGamepad(index, gamepad, false);
}

void GamepadDispatcher::DispatchDidConnectOrDisconnectGamepad(
    unsigned index,
    const device::Gamepad& gamepad,
    bool connected) {
  DCHECK(index < device::Gamepads::kItemsLengthCap);
  DCHECK_EQ(connected, gamepad.connected);

  latest_change_.pad = gamepad;
  latest_change_.index = index;
  NotifyControllers();
}

void GamepadDispatcher::StartListening() {
  Platform::Current()->StartListening(kWebPlatformEventTypeGamepad, this);
}

void GamepadDispatcher::StopListening() {
  Platform::Current()->StopListening(kWebPlatformEventTypeGamepad);
}

}  // namespace blink
