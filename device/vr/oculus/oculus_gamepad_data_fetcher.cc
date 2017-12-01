// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/oculus/oculus_gamepad_data_fetcher.h"

#include <memory>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "device/gamepad/public/cpp/gamepads.h"
#include "third_party/libovr/src/Include/OVR_CAPI.h"
#include "ui/gfx/transform.h"
#include "ui/gfx/transform_util.h"

namespace device {

OculusGamepadDataFetcher::Factory::Factory(unsigned int display_id,
                                           ovrSession session)
    : display_id_(display_id), session_(session) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OculusGamepadDataFetcher::Factory::~Factory() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

std::unique_ptr<GamepadDataFetcher>
OculusGamepadDataFetcher::Factory::CreateDataFetcher() {
  return std::make_unique<OculusGamepadDataFetcher>(display_id_, session_);
}

GamepadSource OculusGamepadDataFetcher::Factory::source() {
  return GAMEPAD_SOURCE_OCULUS;
}

OculusGamepadDataFetcher::OculusGamepadDataFetcher(unsigned int display_id,
                                                   ovrSession session)
    : display_id_(display_id), session_(session) {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

OculusGamepadDataFetcher::~OculusGamepadDataFetcher() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

GamepadSource OculusGamepadDataFetcher::source() {
  return GAMEPAD_SOURCE_OCULUS;
}

void OculusGamepadDataFetcher::OnAddedToProvider() {}

void OculusGamepadDataFetcher::GetGamepadData(bool devices_changed_hint) {
  ovrInputState input_state;
  ovr_GetInputState(
      session_,
      ovrControllerType_Touch,  // TODO(billorr): handle Oculus remote
      &input_state);

  ovrTrackingState tracking_state =
      ovr_GetTrackingState(session_, input_state.TimeInSeconds, false);
  for (int i = 0; i < ovrHand_Count; ++i) {
    PadState* state = GetPadState(i);
    if (!state)
      continue;

    Gamepad& pad = state->data;
    pad.timestamp = input_state.TimeInSeconds;
    pad.connected = true;
    pad.pose.not_null = pad.connected;
    pad.pose.has_orientation = true;
    pad.pose.has_position = true;

    swprintf(pad.id, Gamepad::kIdLengthCap, L"Oculus Controller");
    swprintf(pad.mapping, Gamepad::kMappingLengthCap, L"");
    pad.display_id = display_id_;
    pad.hand = (i == ovrHand_Right) ? GamepadHand::kRight : GamepadHand::kLeft;
    pad.buttons_length = 0;
    pad.axes_length = 4;
    pad.axes[0] = input_state.Thumbstick[i].x;
    pad.axes[1] = input_state.Thumbstick[i].y;
    pad.axes[2] = input_state.HandTrigger[i];
    pad.axes[3] = input_state.IndexTrigger[i];

    if (i == ovrHand_Right) {
      pad.buttons_length = 3;
      pad.buttons[0].pressed = input_state.Buttons & ovrButton_A;
      pad.buttons[0].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[0].touched = input_state.Touches & ovrTouch_A;

      pad.buttons[1].pressed = input_state.Buttons & ovrButton_B;
      pad.buttons[1].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[1].touched = input_state.Touches & ovrTouch_B;

      pad.buttons[2].pressed = input_state.Buttons & ovrButton_RThumb;
      pad.buttons[2].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[2].touched = input_state.Touches & ovrTouch_RThumb;
    } else if (i == ovrHand_Left) {
      pad.buttons_length = 4;
      pad.buttons[0].pressed = input_state.Buttons & ovrButton_X;
      pad.buttons[0].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[0].touched = input_state.Touches & ovrTouch_X;

      pad.buttons[1].pressed = input_state.Buttons & ovrButton_Y;
      pad.buttons[1].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[1].touched = input_state.Touches & ovrTouch_Y;

      pad.buttons[2].pressed = input_state.Buttons & ovrButton_LThumb;
      pad.buttons[2].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[2].touched = input_state.Touches & ovrTouch_LThumb;

      pad.buttons[3].pressed = input_state.Buttons & ovrButton_Enter;
      pad.buttons[3].value = pad.buttons[0].pressed ? 1 : 0;
      pad.buttons[3].touched = pad.buttons[3].pressed;
    }

    pad.pose.orientation.not_null = true;
    pad.pose.orientation.x = tracking_state.HandPoses[i].ThePose.Orientation.x;
    pad.pose.orientation.y = tracking_state.HandPoses[i].ThePose.Orientation.y;
    pad.pose.orientation.z = tracking_state.HandPoses[i].ThePose.Orientation.z;
    pad.pose.orientation.w = tracking_state.HandPoses[i].ThePose.Orientation.w;

    pad.pose.position.not_null = true;
    pad.pose.position.x = tracking_state.HandPoses[i].ThePose.Position.x;
    pad.pose.position.y = tracking_state.HandPoses[i].ThePose.Position.y;
    pad.pose.position.z = tracking_state.HandPoses[i].ThePose.Position.z;

    // TODO(billorr): Set velocity.
    pad.pose.angular_velocity.not_null = false;
    pad.pose.linear_velocity.not_null = false;
  }
}

void OculusGamepadDataFetcher::PauseHint(bool paused) {}

}  // namespace device
